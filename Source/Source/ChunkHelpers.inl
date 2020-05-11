#include "ChunkHelpers.h"
#pragma once
//#include "ChunkHelpers.h"
//#include "chunk.h"

namespace ChunkHelpers
{
	inline localpos worldPosToLocalPos(const glm::ivec3& wpos)
	{
		localpos ret;
		fastWorldPosToLocalPos(wpos, ret);
		return ret;
	}

	inline void fastWorldPosToLocalPos(const glm::ivec3& wpos, localpos& ret)
	{
		// TODO: this is a hack to make the compiler happy
		constexpr int CHUNK_SIZE = 32;

		// compute the modulus of wpos and chunk size (bitwise AND method only works for powers of 2)
		// to find the relative block position in the chunk
		ret.block_pos.x = wpos.x & CHUNK_SIZE - 1;
		ret.block_pos.y = wpos.y & CHUNK_SIZE - 1;
		ret.block_pos.z = wpos.z & CHUNK_SIZE - 1;
		// find the chunk position using integer floor method
		ret.chunk_pos.x = wpos.x / CHUNK_SIZE;
		ret.chunk_pos.y = wpos.y / CHUNK_SIZE;
		ret.chunk_pos.z = wpos.z / CHUNK_SIZE;
		// subtract (floor) if negative w/ non-zero modulus
		if (wpos.x < 0 && ret.block_pos.x) ret.chunk_pos.x--;
		if (wpos.y < 0 && ret.block_pos.y) ret.chunk_pos.y--;
		if (wpos.z < 0 && ret.block_pos.z) ret.chunk_pos.z--;
		// shift local block position forward by chunk size if negative
		if (ret.block_pos.x < 0) ret.block_pos.x += CHUNK_SIZE;
		if (ret.block_pos.y < 0) ret.block_pos.y += CHUNK_SIZE;
		if (ret.block_pos.z < 0) ret.block_pos.z += CHUNK_SIZE;
	}

	inline glm::ivec3 chunkPosToWorldPos(const glm::ivec3& local, const glm::ivec3& cpos)
	{
		// TODO: this is a hack to make the compiler happy
		constexpr int CHUNK_SIZE = 32;
		return glm::ivec3(local + (cpos * CHUNK_SIZE));
	}

	// clockwise from bottom left texture coordinates
	inline const glm::vec2 tex_corners[] =
	{
		{ 0, 0 },
		{ 0, 1 },
		{ 1, 1 },
		{ 1, 0 }
	};

	inline GLuint Encode(const glm::uvec3& modelPos, GLuint normalIdx, GLuint texIdx, GLuint cornerIdx)
	{
		GLuint encoded = 0;

		// encode vertex position
		encoded |= modelPos.x << 26;
		encoded |= modelPos.y << 20;
		encoded |= modelPos.z << 14;

		// encode normal
		encoded |= normalIdx << 11;

		// encode texture information
		encoded |= texIdx << 2;
		encoded |= cornerIdx << 0;

		return encoded;
	}


	inline void Decode(GLuint encoded, glm::uvec3& modelPos, glm::vec3& normal, glm::vec2& texCoord)
	{
		// decode vertex position
		modelPos.x = encoded >> 26;
		modelPos.y = (encoded >> 20) & 0x3F; // = 0b111111
		modelPos.z = (encoded >> 14) & 0x3F; // = 0b111111

		// decode normal
		GLuint normalIdx = (encoded >> 11) & 0x7; // = 0b111
		normal = faces[normalIdx];

		// decode texture index and UV
		GLuint textureIdx = (encoded >> 2) & 0x1FF; // = 0b1111111111
		GLuint cornerIdx = (encoded >> 0) & 0x3; // = 0b11
		glm::vec2 corner = tex_corners[cornerIdx];

		// sample from texture using knowledge of texture dimensions and block index
		// texCoord = ...
	}


	inline GLuint EncodeSplat(const glm::uvec3& modelPos, const glm::vec3& color)
	{
		GLuint encoded = 0;

		// encode vertex position
		encoded |= modelPos.x << 26;
		encoded |= modelPos.y << 20;
		encoded |= modelPos.z << 14;

		// encode color as 4-bit unsigned int
		glm::uvec3 tmp;
		tmp.r = unsigned(color.r * (1 << 4));
		tmp.g = unsigned(color.g * (1 << 4));
		tmp.b = unsigned(color.b * (1 << 4));
		tmp = glm::clamp(tmp, glm::uvec3(0), glm::uvec3(15));
		encoded |= tmp.r << 10;
		encoded |= tmp.g << 6;
		encoded |= tmp.b << 2;

		return encoded;
	}
}