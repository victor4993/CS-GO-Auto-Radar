#pragma once

// Nodes for VMF files

#include "vmf.hpp"
#include "CompositorFrame.hpp"
#include "GBuffer.hpp"

namespace TARCF {
	Shader* s_shader_gbuf_debug;
	Shader* s_shader_mask_write;
	Shader* s_shader_compheight;

	// Fast guassian blur implementation
	class RenderBrushesHeight: public BaseNode {
	public:
		RenderBrushesHeight() :
			BaseNode(GBuffer::s_gbufferwrite_cleanShader)
		{
			m_prop_definitions.insert({ "vmf", prop::prop_explicit<vmf*>(EXTRATYPE_RAWPTR, 0, -1) });
			m_prop_definitions.insert({ "layers", prop::prop_explicit<unsigned int>(GL_INT, TAR_CHANNEL_NONRESERVE, -1) });
			m_prop_definitions.insert({ "matrix.view", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });
			m_prop_definitions.insert({ "matrix.proj", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });

			m_output_definitions.push_back(Pin("GBuffer", 0));
		}

		void compute(NodeInstance* node) override {
			glViewport(0, 0, node->m_gl_texture_w, node->m_gl_texture_h);
			glBindFramebuffer(GL_FRAMEBUFFER, node->m_gl_framebuffers[0]);
			
			// Clear position buffer Y coordinate to very high value
			glClearColor(0, 99999.9f, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//LOG_F(INFO, "rendering vmf");

			GBuffer::s_gbufferwrite_cleanShader->use();

			// Set matrices
			GBuffer::s_gbufferwrite_cleanShader->setMatrix("view", node->m_properties["matrix.view"].getValue<glm::mat4>());
			GBuffer::s_gbufferwrite_cleanShader->setMatrix("projection", node->m_properties["matrix.proj"].getValue<glm::mat4>());

			unsigned int mask1 = node->m_properties["layers"].getValue<unsigned int>();
			TARChannel::setChannels(mask1 | TAR_CHANNEL_MASK);

			// Draw vmf
			node->m_properties["vmf"].getValue<vmf*>()->DrawWorld(GBuffer::s_gbufferwrite_cleanShader, glm::mat4(1.0f), [](solid* ptrSolid, entity* ptrEnt) {
			}, true);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void v_gen_buffers(NodeInstance* instance) override {
			// Front and back buffer
			glGenFramebuffers(1, &instance->m_gl_framebuffers[0]);
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
		}

		void v_gen_tex_memory(NodeInstance* instance) override {
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
			glGenTextures(2, &instance->m_gl_texture_ids[0]);

			// Position buffer float16 (64bpp) a channel for masking.
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RED, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance->m_gl_texture_ids[0], 0);

			// Depth texture
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, instance->m_gl_texture_ids[1], 0);

			unsigned int attachments[1] = {
				GL_COLOR_ATTACHMENT0
			};

			glDrawBuffers(1, attachments);
		}

		void debug_fs(const NodeInstance* instance, int channel) override {
			s_shader_gbuf_debug->use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			s_mesh_quad->Draw();
		}

		uint64_t get_tex_memory_usage(const NodeInstance* instance) override {
			return instance->m_gl_texture_w * instance->m_gl_texture_h * (3 + 3 + 3 + 3);
		}
	};

	class RenderBrushesGBuffer : public BaseNode {
	public:
		RenderBrushesGBuffer() :
			BaseNode(GBuffer::s_gbufferwriteShader)
		{
			m_prop_definitions.insert({ "vmf", prop::prop_explicit<vmf*>(EXTRATYPE_RAWPTR, 0, -1) });
			m_prop_definitions.insert({ "layers", prop::prop_explicit<unsigned int>(GL_INT, TAR_CHANNEL_NONRESERVE, -1) });
			m_prop_definitions.insert({ "matrix.view", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });
			m_prop_definitions.insert({ "matrix.proj", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });

			m_output_definitions.push_back(Pin("GBuffer", 0));
			m_output_definitions.push_back(Pin("NBuffer", 1));
			m_output_definitions.push_back(Pin("OBuffer", 2));
		}

		void compute(NodeInstance* node) override {
			glViewport(0, 0, node->m_gl_texture_w, node->m_gl_texture_h);
			glBindFramebuffer(GL_FRAMEBUFFER, node->m_gl_framebuffers[0]);

			// Clear position buffer Y coordinate to very high value
			glClearColor(0, 99999.9f, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//LOG_F(INFO, "rendering vmf");

			GBuffer::s_gbufferwriteShader->use();

			// Set matrices
			GBuffer::s_gbufferwriteShader->setMatrix("view", node->m_properties["matrix.view"].getValue<glm::mat4>());
			GBuffer::s_gbufferwriteShader->setMatrix("projection", node->m_properties["matrix.proj"].getValue<glm::mat4>());

			unsigned int mask1 = node->m_properties["layers"].getValue<unsigned int>();
			TARChannel::setChannels(mask1 | TAR_CHANNEL_MASK);

			// Draw vmf
			node->m_properties["vmf"].getValue<vmf*>()->DrawWorld(GBuffer::s_gbufferwriteShader, glm::mat4(1.0f), [](solid* ptrSolid, entity* ptrEnt) {
				if (ptrSolid) {
					glm::vec3 orig = (ptrSolid->m_bounds.MAX + ptrSolid->m_bounds.MIN) * 0.5f;
					GBuffer::s_gbufferwriteShader->setVec3("srcOrigin", glm::vec3(orig.x, orig.y, orig.z));
				}
				if (ptrEnt) {
					GBuffer::s_gbufferwriteShader->setVec3("srcOrigin", glm::vec3(0, 0, 0));
				}
			}, true);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void v_gen_buffers(NodeInstance* instance) override {
			// Front and back buffer
			glGenFramebuffers(1, &instance->m_gl_framebuffers[0]);
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
		}

		void v_gen_tex_memory(NodeInstance* instance) override {
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
			glGenTextures(4, &instance->m_gl_texture_ids[0]);

			// Position buffer float16 (64bpp) a channel for masking.
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance->m_gl_texture_ids[0], 0);

			// normal buffer float16 (48bpp)
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, instance->m_gl_texture_ids[1], 0);

			// Brush/model origin whatever
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[2]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, instance->m_gl_texture_ids[2], 0);

			// Depth texture
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[3]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, instance->m_gl_texture_ids[3], 0);

			unsigned int attachments[3] = {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2
			};

			glDrawBuffers(3, attachments);
		}

		void debug_fs(const NodeInstance* instance, int channel) override {
			s_shader_gbuf_debug->use();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			s_mesh_quad->Draw();
		}

		uint64_t get_tex_memory_usage(const NodeInstance* instance) override {
			return instance->m_gl_texture_w * instance->m_gl_texture_h * (3 + 3 + 3 + 3);
		}
	};

	class RenderBrushesMask: public BaseNode {
	public:
		RenderBrushesMask() :
			BaseNode(s_shader_mask_write)
		{
			m_prop_definitions.insert({ "vmf", prop::prop_explicit<vmf*>(EXTRATYPE_RAWPTR, 0, -1) });
			m_prop_definitions.insert({ "layers", prop::prop_explicit<unsigned int>(GL_INT, TAR_CHANNEL_NONRESERVE, -1) });
			m_prop_definitions.insert({ "matrix.view", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });
			m_prop_definitions.insert({ "matrix.proj", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });

			m_output_definitions.push_back(Pin("Mask", 0));
		}

		void compute(NodeInstance* node) override {
			glViewport(0, 0, node->m_gl_texture_w, node->m_gl_texture_h);
			glBindFramebuffer(GL_FRAMEBUFFER, node->m_gl_framebuffers[0]);

			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//LOG_F(INFO, "rendering vmf");

			TARChannel::setChannels(node->m_properties["layers"].getValue<unsigned int>() | TAR_CHANNEL_MASK);
			//LOG_F(INFO, "Tarchannel render limit set: %u", node->m_properties["layers"].getValue<unsigned int>());
			s_shader_mask_write->use();

			// Set matrices
			s_shader_mask_write->setMatrix("view", node->m_properties["matrix.view"].getValue<glm::mat4>());
			s_shader_mask_write->setMatrix("projection", node->m_properties["matrix.proj"].getValue<glm::mat4>());

			// Draw vmf
			node->m_properties["vmf"].getValue<vmf*>()->DrawWorld(s_shader_mask_write, glm::mat4(1.0f), [](solid* ptrSolid, entity* ptrEnt) {
				s_shader_mask_write->setVec3("color", glm::vec3(1, 1, 1));
				if (ptrSolid)
					if (ptrSolid->isShown(TAR_CHANNEL_MASK))
						s_shader_mask_write->setVec3("color", glm::vec3(0, 0, 0));
				if (ptrEnt)
					if (ptrEnt->isShown(TAR_CHANNEL_MASK))
						s_shader_mask_write->setVec3("color", glm::vec3(0, 0, 0));
			});

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void v_gen_buffers(NodeInstance* instance) override {
			// Front and back buffer
			glGenFramebuffers(1, &instance->m_gl_framebuffers[0]);
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
		}

		void v_gen_tex_memory(NodeInstance* instance) override {
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
			glGenTextures(2, &instance->m_gl_texture_ids[0]);

			// Position buffer float16 (64bpp) a channel for masking.
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance->m_gl_texture_ids[0], 0);

			// Depth texture
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, instance->m_gl_texture_ids[1], 0);

			unsigned int attachments[1] = {
				GL_COLOR_ATTACHMENT0
			};

			glDrawBuffers(1, attachments);
		}

		uint64_t get_tex_memory_usage(const NodeInstance* instance) override {
			return instance->m_gl_texture_w * instance->m_gl_texture_h * ( 3 + 1 );
		}
	};

	class RelativeHeight : public BaseNode {
	public:
		RelativeHeight()
			: BaseNode(s_shader_compheight)
		{
			m_input_definitions.push_back(Pin("gPos", 0));
			m_input_definitions.push_back(Pin("gOrigin", 1));
			m_input_definitions.push_back(Pin("gRefHeight", 2));
			m_input_definitions.push_back(Pin("gRefHeight1", 2));

			m_output_definitions.push_back(Pin("output", 0));

			m_prop_definitions.insert({ "matrix.view", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });
			m_prop_definitions.insert({ "matrix.proj", prop::prop_explicit<glm::mat4>(GL_FLOAT_MAT4, glm::mat4(1.0), -1) });
		}

		void v_gen_tex_memory(NodeInstance* instance) override {
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_gl_framebuffers[0]);
			glGenTextures(1, &instance->m_gl_texture_ids[0]);

			// Position buffer float16
			glBindTexture(GL_TEXTURE_2D, instance->m_gl_texture_ids[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, instance->m_gl_texture_w, instance->m_gl_texture_h, 0, GL_RED, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, instance->m_gl_texture_ids[0], 0);

			unsigned int attachments[1] = {
				GL_COLOR_ATTACHMENT0
			};

			glDrawBuffers(1, attachments);
		}

		void compute(NodeInstance* node) override {
			glViewport(0, 0, node->m_gl_texture_w, node->m_gl_texture_h);
			
			// Bind shader
			m_operator_shader->use();
			m_operator_shader->setMatrix("projection", node->m_properties["matrix.proj"].getValue<glm::mat4>());
			m_operator_shader->setMatrix("view", node->m_properties["matrix.view"].getValue<glm::mat4>());

			m_operator_shader->setInt("gPos", 0);
			m_operator_shader->setInt("gOrigin", 1);
			m_operator_shader->setInt("refPos", 2);
			m_operator_shader->setInt("refPos1", 3);

			glBindFramebuffer(GL_FRAMEBUFFER, node->m_gl_framebuffers[0]);

			s_mesh_quad->Draw();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// 2 byte/pixel
		uint64_t get_tex_memory_usage(const NodeInstance* instance) override {
			return instance->m_gl_texture_w * instance->m_gl_texture_h * 2;
		}
	};

	void VMF_NODES_INIT() {
		// Create debug shader
		s_shader_gbuf_debug = new Shader("shaders/engine/quadbase.vs", "shaders/engine/node.gbuffer.preview.fs", "shader.node.gbuffer.preview");
		s_shader_mask_write = new Shader("shaders/unlit.vs", "shaders/unlit.fs", "unlit.duplicate(1)");
		s_shader_compheight = new Shader("shaders/engine/quadbase.vs", "shaders/engine/gb.hcomp.fs", "ref.height");

		// Append to node lib
		NODELIB.insert({ "vmf.gbuffer", new RenderBrushesGBuffer() });
		NODELIB.insert({ "vmf.height", new RenderBrushesHeight() });
		NODELIB.insert({ "vmf.mask",	new RenderBrushesMask() });
		NODELIB.insert({ "vmf.relativeheight", new RelativeHeight() });
	}
}