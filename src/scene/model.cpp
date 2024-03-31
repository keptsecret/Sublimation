#include <scene/model.h>

#include <graphics/vulkan/rendering_device.h>
#include <graphics/vulkan/utils.h>

#include <array>
#include <assimp/Importer.hpp>
#include <iostream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

namespace sublimation {

Mesh::Mesh(glm::mat4 matrix) {
}

Mesh::~Mesh() {
}

glm::mat4 Node::getLocalTransform() {
    return glm::translate(translation) * glm::rotate(rotation.x, glm::vec3(1, 0, 0)) * glm::rotate(rotation.y, glm::vec3(0, 1, 0)) * glm::rotate(rotation.z, glm::vec3(0, 0, 1)) * glm::scale(scale);
}

glm::mat4 Node::getWorldTransform() {
    glm::mat4 m = getLocalTransform();
    auto p = parent;
    while (p) {
        m = p->getLocalTransform() * m;
        p = p->parent;
    }
    return m;
}

void Node::update() {
    if (mesh) {
        glm::mat4 m = getWorldTransform();
        mesh->pushConstants.model = m;
    }

    for (auto& child : children) {
        child->update();
    }
}

Node::~Node() {
}

VkVertexInputBindingDescription Vertex::getBindingDescription(uint32_t binding) {
    return VkVertexInputBindingDescription{
        .binding = binding,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions(uint32_t binding) {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = binding;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = binding;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = binding;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    attributeDescriptions[3].binding = binding;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, tangent);

    return attributeDescriptions;
}

void Model::loadFromFile(const std::string& filepath, uint32_t postProcessFlags) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, postProcessFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::Model:loadFromFile: " << importer.GetErrorString() << '\n';
        return;
    }

    loadFromAiScene(scene, filepath);
}

void Model::loadFromAiScene(const aiScene* scene, const std::string& filepath) {
    path = filepath.substr(0, filepath.find_last_of('/'));

    loadMaterials(scene);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    processNode(scene->mRootNode, scene, nullptr, vertices, indices);

    for (auto& node : linearNodes) {
        if (node->mesh) {
            node->update();
        }
    }

    uint32_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    uint32_t indexBufferSize = indices.size() * sizeof(uint32_t);
    vkw::RenderingDevice* rd = vkw::RenderingDevice::getSingleton();

    // Vertex data
    {
        vkw::Buffer vertexStagingBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, vertices.data());

        vertexBuffer = rd->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBufferSize);
        VkCommandBuffer commandBuffer = rd->getCommandBufferOneTime(0 /*TODO: change to current command pool index */);

        VkBufferCopy copyRegion{
            .size = vertexBufferSize
        };
        if (auto buffer = vertexBuffer.lock()) {
            vkCmdCopyBuffer(commandBuffer, vertexStagingBuffer.getBuffer(), buffer->getBuffer(), 1, &copyRegion);
        }

        rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
    }

    // Index data
    {
        vkw::Buffer indexStagingBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, indices.data());

        indexBuffer = rd->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, indexBufferSize);

        VkCommandBuffer commandBuffer = rd->getCommandBufferOneTime(0 /*TODO: change to current command pool index */);

        VkBufferCopy copyRegion{
            .size = indexBufferSize
        };
        if (auto buffer = indexBuffer.lock()) {
            vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.getBuffer(), buffer->getBuffer(), 1, &copyRegion);
        }

        rd->commandBufferSubmitIdle(&commandBuffer, VK_QUEUE_GRAPHICS_BIT);
    }

    //updateModelBounds();

    for (auto& material : materials) {
        if (material->textures[0].isActive()) {
            material->updateDescriptors();
        }
    }
}

void Model::loadMaterials(const aiScene* scene) {
    for (size_t i = 0; i < scene->mNumMaterials; i++) {
        const aiMaterial* aimaterial = scene->mMaterials[i];

        std::unique_ptr<Material> newMaterial = std::make_unique<Material>();

        // Load material values
        aiColor3D color;
        if (aimaterial->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
            newMaterial->albedo = glm::vec4(color.r, color.g, color.b, 1);
        } else if (aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            newMaterial->albedo = glm::vec4(color.r, color.g, color.b, 1);
        }

        if (aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, newMaterial->metallicRoughnessOcclusionFactor.x) != AI_SUCCESS) {
            aimaterial->Get(AI_MATKEY_SPECULAR_FACTOR, newMaterial->metallicRoughnessOcclusionFactor.x);
        }
        if (aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, newMaterial->metallicRoughnessOcclusionFactor.y) != AI_SUCCESS) {
            if (aimaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, newMaterial->metallicRoughnessOcclusionFactor.y) == AI_SUCCESS) {
                newMaterial->aux.roughnessGlossyMode = 1;
            }
        }

        // Load material textures, nullptr if not
        newMaterial->textures[0] = loadTexture(aimaterial, aiTextureType_DIFFUSE);
        newMaterial->textures[1] = loadTexture(aimaterial, aiTextureType_METALNESS);
        if (!newMaterial->textures[1].isActive()) {
            newMaterial->textures[1] = loadTexture(aimaterial, aiTextureType_SPECULAR);
        }
        newMaterial->textures[2] = loadTexture(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS);
        newMaterial->textures[3] = loadTexture(aimaterial, aiTextureType_AMBIENT_OCCLUSION);

        newMaterial->textures[4] = loadTexture(aimaterial, aiTextureType_NORMALS);

        if (newMaterial->textures[4].isActive()) {
            newMaterial->aux.normalMapMode = 1;
        } else {
            // no normal map found, try loading the bump map instead
            newMaterial->textures[4] = loadTexture(aimaterial, aiTextureType_HEIGHT);
            if (newMaterial->textures[4].isActive()) {
                newMaterial->aux.normalMapMode = 2;
            }
        }

        newMaterial->apply();

        materials.push_back(std::move(newMaterial));
    }
}

Texture Model::loadTexture(const aiMaterial* mat, aiTextureType type) {
    Texture texture;
    uint32_t typeCount = mat->GetTextureCount(type);
    if (typeCount == 0) {
        std::cout << "INFO::Model:loadTexture: found no textures for material " << mat->GetName().C_Str()
                  << "of type " << aiTextureTypeToString(type) << "\n";
        return texture;
    } else if (typeCount > 1) {
        std::cout << "INFO::Model:loadTexture: found more than 1 texture for material " << mat->GetName().C_Str()
                  << "of type " << aiTextureTypeToString(type) << ", selecting only first one\n";
    }

    aiString filepath;
    mat->GetTexture(type, 0, &filepath);
    bool foundLoaded = false;
    for (unsigned int i = 0; i < textures.size(); i++) {
        if (std::strcmp(textures[i].filepath.c_str(), filepath.C_Str()) == 0) {
            texture = textures[i];
            foundLoaded = true;
            break;
        }
    }

    if (!foundLoaded) {
        texture.filepath = filepath.C_Str();
        texture.texture = vkw::RenderingDevice::getSingleton()->loadTextureFromFile(path + '/' + filepath.C_Str(),
            VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, false, false);
        textures.push_back(texture);
    }

    return texture;
}

void Model::processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> parent, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    std::shared_ptr<Node> newNode = std::make_shared<Node>();
    newNode->parent = parent.get();
    newNode->name = node->mName.C_Str();

    aiVector3t<float> translate, rotate, scale;
    node->mTransformation.Decompose(scale, rotate, translate);
    newNode->translation = glm::vec3(translate.x, translate.y, translate.z);
    newNode->rotation = glm::vec3(glm::degrees(rotate.x), glm::degrees(rotate.y), glm::degrees(rotate.z));
    newNode->scale = glm::vec3(scale.x, scale.y, scale.z);
    newNode->transform = glm::mat4(node->mTransformation.a1, node->mTransformation.a2, node->mTransformation.a3, node->mTransformation.a4,
            node->mTransformation.b1, node->mTransformation.b2, node->mTransformation.b3, node->mTransformation.b4,
            node->mTransformation.c1, node->mTransformation.c2, node->mTransformation.c3, node->mTransformation.c4,
            node->mTransformation.d1, node->mTransformation.d2, node->mTransformation.d3, node->mTransformation.d4);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        std::shared_ptr<Node> childNode = std::make_shared<Node>();
        childNode->parent = newNode.get();
        childNode->name = mesh->mName.C_Str();
        childNode->transform = glm::mat4(1.f);

        childNode->mesh = processMesh(mesh, scene, newNode->transform, vertices, indices); // TODO: check transform param
        newNode->children.push_back(childNode);
        linearNodes.push_back(childNode.get());
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, newNode, vertices, indices);
    }

    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
    linearNodes.push_back(newNode.get());
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4& transform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    std::unique_ptr<Mesh> newMesh = std::make_unique<Mesh>(transform);
    newMesh->name = mesh->mName.C_Str();

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{
            .position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
            .normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
            .uv = glm::vec2(0.f)
        };

        if (mesh->mTangents) {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }

        if (mesh->mTextureCoords[0]) {
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        vertices.push_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        uint32_t firstIndex = indices.size();
        uint32_t indexCount = face.mNumIndices;

        glm::vec3 pmin{ FLT_MAX }, pmax{ -FLT_MAX };
        //AABB aabb{};
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);

            glm::vec3 pos = vertices[face.mIndices[j]].position;
        }

        std::shared_ptr<Triangle> triangle = std::make_shared<Triangle>();
        triangle->firstIndex = firstIndex;
        triangle->indexCount = indexCount;
        triangle->material = mesh->mMaterialIndex > -1 ? materials[mesh->mMaterialIndex].get() : materials.back().get();
        //triangle->bounds = aabb;
        newMesh->primitives.push_back(triangle);
    }

    return newMesh;
}

//void Model::updateModelBounds() {
//    glm::vec3 pMin{ FLT_MAX };
//    glm::vec3 pMax{ -FLT_MAX };
//    for (auto& node : nodes) {
//        updateNodeBounds(node.get(), pMin, pMax);
//    }
//    //bounds = AABB{ pMin, pMax };
//}
//
//void Model::updateNodeBounds(Node* node, glm::vec3& pmin, glm::vec3& pmax) {
//    if (node->mesh) {
//        for (auto& triangle : node->mesh->primitives) {
//            //bounds.transform(node->getWorldTransform());
//            glm::vec3 nodeMin = triangle->bounds.min();
//            glm::vec3 nodeMax = triangle->bounds.max();
//            if (nodeMin.x < pmin.x) {
//                pmin.x = nodeMin.x;
//            }
//            if (nodeMin.y < pmin.y) {
//                pmin.y = nodeMin.y;
//            }
//            if (nodeMin.z < pmin.z) {
//                pmin.z = nodeMin.z;
//            }
//            if (nodeMax.x > pmax.x) {
//                pmax.x = nodeMax.x;
//            }
//            if (nodeMax.y > pmax.y) {
//                pmax.y = nodeMax.y;
//            }
//            if (nodeMax.z > pmax.z) {
//                pmax.z = nodeMax.z;
//            }
//        }
//    }
//    for (auto& child : node->children) {
//        updateNodeBounds(child.get(), pmin, pmax);
//    }
//}

void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t renderFlags, uint32_t bindImageset) {
    const VkDeviceSize offsets[1] = { 0 };
    if (auto buffer = vertexBuffer.lock()) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer->getBuffer(), offsets);
    }
    if (auto buffer = indexBuffer.lock()) {
        vkCmdBindIndexBuffer(commandBuffer, buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    for (auto& node : nodes) {
        drawNode(node, commandBuffer, pipelineLayout, renderFlags, bindImageset);
    }
}

void Model::drawNode(std::shared_ptr<Node> node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t renderFlags, uint32_t bindImageset) {
    if (node->mesh) {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &node->mesh->pushConstants);
        for (auto primitive : node->mesh->primitives) {
            if (renderFlags & RenderFlag::BindImages) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindImageset,
                    static_cast<uint32_t>(primitive->material->descriptorSet.getDescriptorSets().size()), 
                    primitive->material->descriptorSet.getDescriptorSets().data(), 0, nullptr);
            }

            vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
    }

    for (auto& child : node->children) {
        drawNode(child, commandBuffer, pipelineLayout, renderFlags, bindImageset);
    }
}

Model::~Model() {
    for (auto& material : materials) {
        material.reset();
    }
}

} //namespace sublimation