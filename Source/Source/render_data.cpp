#include "stdafx.h"
#include "component.h"
#include "ibo.h"
#include "vbo.h"
#include "vao.h"
#include "texture.h"
#include "pipeline.h"
#include "shader.h"

#include "render_data.h"

Component * RenderData::Clone() const
{
	RenderDataPtr rend = new RenderData(*this);
	rend->SetType(ComponentType::cRenderData);
	return rend;
}

// generate a new set of stuff to use
// this method sucks (should instead be using a list of presets defined in render)
void RenderData::UseUntexturedBlockData()
{
	if (vao_)
		delete vao_;
	if (vbo_)
		delete vbo_;
	if (ibo_)
		delete ibo_;

	vbo_ = new VBO(Render::cube_tex_vertices, sizeof(Render::cube_tex_vertices));
	vao_ = new VAO();
	VBOlayout layout;
	layout.Push<GLfloat>(3);
	//layout.Push<GLfloat>(3);
	layout.Push<GLfloat>(2);
	vao_->AddBuffer(*vbo_, layout);

	shader_ = Shader::shaders["flat"];
}