#pragma once

class Frustum;

enum CameraType : int
{
	kControlCam, // use movement keys to fly around
	kAffixedCam, // parented to some object
	kImmobileCam // cannot move
};

class Camera
{
public:
	Camera(CameraType type);
	void Update(float dt);
	inline const glm::mat4& GetView() const { return view_; }
	inline const glm::mat4& GetProj() const { return proj_; }
	inline const glm::vec3& GetPos() const { return worldpos_; }
	inline const Frustum* GetFrustum() const { return frustum_; }
	inline const float GetFov() const { return fov_; }

	inline void SetPos(const glm::vec3& v) { worldpos_ = v; }

	glm::vec3 up = glm::vec3(0, 1.f, 0);
	glm::vec3 front = glm::vec3(0, 0, -1.f);
private:
	CameraType type_;
	glm::vec3 worldpos_ = glm::vec3(0, 0, 0);
	glm::mat4 view_ = glm::mat4(1);
	glm::mat4 proj_;
	Frustum* frustum_;

	float speed_ = 3.5f;

	float pitch_ = 0;
	float yaw_ = -90.0f;
	float roll_ = 0;
	float fov_ = 80.f;
};