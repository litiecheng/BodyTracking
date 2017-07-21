#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics1/Color.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/System.h>
#include <Kore/Log.h>

#include "MeshObject.h"
#include "RotationUtility.h"

#ifdef KORE_STEAMVR
#include <Kore/Vr/VrInterface.h>
#include <Kore/Vr/SensorState.h>
#endif

using namespace Kore;
using namespace Kore::Graphics4;

namespace {
	
#ifdef KORE_STEAMVR
	const int width = 2048;
	const int height = 1024;
#else
	const int width = 1024;
	const int height = 768;
#endif
	
	double startTime;
	double lastTime;
	
	VertexStructure structure;
	Shader* vertexShader;
	Shader* fragmentShader;
	PipelineState* pipeline;
	PipelineState* pipeline2;
	
	// Uniform locations
	TextureUnit tex;
	ConstantLocation pLocation;
	ConstantLocation vLocation;
	ConstantLocation mLocation;
	
	bool left, right = false;
	bool down, up = false;
	bool forward, backward = false;
	bool rotateX = false;
	bool rotateY = false;
	bool rotateZ = false;
	int mousePressX, mousePressY = 0;
	
	MeshObject* cube;
	MeshObject* avatar;
	
	bool rotateCube = false;
	bool rotateCubeX = false;
	bool rotateCubeY = false;
	bool rotateCubeZ = false;
	
#ifdef KORE_STEAMVR
	vec3 globe = vec3(Kore::pi, 0, 0);
	vec3 playerPosition = vec3(0, 0, 0);
#else
	vec3 globe = vec3(0, 0, 0);
	vec3 playerPosition = vec3(0, 0.7, 1.5);
#endif
	int frame = 0;
	
	float angle = 0;
	vec3 desPosition = vec3(0, 0, 0);
	Quaternion desRotation = Quaternion(0, 0, 0, 1);
	mat4 matrix = mat4::Identity();

	mat4 T = mat4::Identity();
	mat4 initTrans = mat4::Identity();
	mat4 hmsOffset = mat4::Translation(0, 0.2, 0);
	mat4 initRot = mat4::Identity();
	mat4 initRotInv = mat4::Identity();
	
	bool initCharacter = false;

	const int targetBoneIndex = 10;	// Left foot 49, right foot 53, Left hand 10, right hand 29
	const int renderTrackerOrTargetPosition = 1;		// 0 - dont render, 1 - render desired position, 2 - render target position

	void renderTracker() {
		switch (renderTrackerOrTargetPosition) {
			case 0:
				// Dont render
				break;
			case 1:
			{
				// Render desired position
/*#ifdef KORE_STEAMVR
				vec4 cubePos = vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
#else
				vec4 cubePos = avatar->M * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
#endif*/
				cube->M = mat4::Translation(desPosition.x(), desPosition.y(), desPosition.z()) * desRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube->M);
				//Graphics4::setPipeline(pipeline2);
				cube->render(tex);
				break;
			}
			case 2:
			{
				// Render target position
				vec3 targetPosition = avatar->getBonePosition(targetBoneIndex);
				Quaternion targetRotation = avatar->getBoneGlobalRotation(targetBoneIndex);
				cube->M = avatar->M * mat4::Translation(targetPosition.x(), targetPosition.y(), targetPosition.z()) * targetRotation.matrix().Transpose();
				Graphics4::setMatrix(mLocation, cube->M);
				//Graphics4::setPipeline(pipeline2);
				cube->render(tex);
				break;
			}
			default:
				break;
		}
	}
	
	void update() {
		float t = (float)(System::time() - startTime);
		double deltaT = t - lastTime;
		lastTime = t;
		
		const float speed = 0.1f;
		if (left) {
			playerPosition.x() -= speed;
		}
		if (right) {
			playerPosition.x() += speed;
		}
		if (forward) {
			playerPosition.z() += speed;
		}
		if (backward) {
			playerPosition.z() -= speed;
		}
		if (up) {
			playerPosition.y() += speed;
		}
		if (down) {
			playerPosition.y() -= speed;
		}
		
		if (rotateCube) {
			vec3 currentRotation;
			
			if (rotateCubeX) currentRotation.x() += 0.01;
			if (rotateCubeY) currentRotation.y() += 0.01;
			if (rotateCubeZ) currentRotation.z() -= 0.01;
			
			mat4 rot = desRotation.matrix().Transpose() * mat4::Rotation(currentRotation.x(), currentRotation.y(), currentRotation.z());
			RotationUtility::getOrientation(&rot, &desRotation);
			
			//rotateCube = false;
		}

		
		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);
		
		Graphics4::setPipeline(pipeline);

#ifdef KORE_STEAMVR

		bool firstPersonVR = true;
		bool firstPersonMonitor = false;

		VrInterface::begin();
		SensorState state;

		if (!initCharacter) {
			float currentAvatarHeight = avatar->getHeight();

			state = VrInterface::getSensorState(0);
			vec3 hmdPos = state.pose.vrPose.position; // z -> face, y -> up down
			float currentUserHeight = hmdPos.y();

			//playerPosition.x() = -currentUserHeight * 0.5;
			playerPosition.y() = currentUserHeight * 1.5;
			playerPosition.z() = currentUserHeight * 0.5;
			playerPosition = vec3(0.000000, 0.734777, 0.794926);

			float scale = currentUserHeight / currentAvatarHeight;
//			avatar->setScale(scale);

			// Set initial transformation
			initTrans = mat4::Translation(hmdPos.x(), 0, hmdPos.z());
			
			// Set initial orientation
			Quaternion hmdOrient = state.pose.vrPose.orientation;
			float zAngle = 2 * Kore::acos(hmdOrient.y);
			initRot *= mat4::RotationZ(-zAngle);

			avatar->M = initTrans * initRot * hmsOffset;
			cube->M = avatar->M;
			T = (initTrans * initRot * hmsOffset).Invert();

			log(Info, "current avatar height %f, currend user height %f, scale %f", currentAvatarHeight, currentUserHeight, scale);

			initCharacter = true;
		}

		VrPoseState controller;
		for (int i = 0; i < 16; ++i) {
			controller = VrInterface::getController(i);
			if (controller.trackedDevice == TrackedDevice::ViveTracker) break;
		}

		// Get controller position
		desPosition = controller.vrPose.position;
		vec4 finalPos = T * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(targetBoneIndex, finalPos);

		// Get controller rotation
		desRotation = controller.vrPose.orientation;
		//avatar->setDesiredPositionAndOrientation(targetBoneIndex, finalPos, desRotation);
		
		for (int eye = 0; eye < 2; ++eye) {
			VrInterface::beginRender(eye);

			Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

			state = VrInterface::getSensorState(eye);
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);

			// Render
			Graphics4::setMatrix(mLocation, avatar->M);
			avatar->animate(tex, deltaT);

			renderTracker();

			VrInterface::endRender(eye);
		}

		VrInterface::warpSwap();

		Graphics4::restoreRenderTarget();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, Graphics1::Color::Black, 1.0f, 0);

		// Render
		if (!firstPersonMonitor) {
			mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
			P.Set(0, 0, -P.get(0, 0));

			// view matrix
			vec3 lookAt = playerPosition + vec3(0, 0, -1);
			mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
			V *= mat4::Rotation(globe.x(), globe.y(), globe.z());

			Graphics4::setMatrix(vLocation, V);
			Graphics4::setMatrix(pLocation, P);
		} else {
			Graphics4::setMatrix(vLocation, state.pose.vrPose.eye);
			Graphics4::setMatrix(pLocation, state.pose.vrPose.projection);
		}
		Graphics4::setMatrix(mLocation, avatar->M);
		avatar->animate(tex, deltaT);

		renderTracker();
		Graphics4::setPipeline(pipeline);

		//cube->drawVertices(cube->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height);
		//avatar->drawJoints(avatar->M, state.pose.vrPose.eye, state.pose.vrPose.projection, width, height, true);

#else
		// Scale test
		if (!initCharacter) {
			//avatar->setScale(0.8);
			avatar->M = initTrans * initRot;
			T = (initTrans * initRot).Invert();
			initCharacter = true;
			
			avatar->animate(tex, deltaT);
			
			//desRotation = avatar->getBoneGlobalRotation(targetBoneIndex);
			
			//mat4 rot = mat4::RotationZ(Kore::pi/2) * desRotation.matrix().Transpose();
			//RotationUtility::getOrientation(&rot, &desRotation);

		}
		
		// projection matrix
		mat4 P = mat4::Perspective(45, (float)width / (float)height, 0.01f, 1000);
		
		// view matrix
		vec3 lookAt = playerPosition + vec3(0, 0, -1);
		mat4 V = mat4::lookAt(playerPosition, lookAt, vec3(0, 1, 0));
		V *= mat4::Rotation(globe.x(), globe.y(), globe.z());
		
		Graphics4::setMatrix(vLocation, V);
		Graphics4::setMatrix(pLocation, P);
		
		Graphics4::setMatrix(mLocation, avatar->M);
		/*avatar->setAnimation(frame);
		frame++;
		if (frame > 200) frame = 0;*/                                                                                                                                                            
		avatar->animate(tex, deltaT);
		
		angle += 0.05;
		float radius = 0.2;
		
		// Set foot position
		desPosition = vec3(-0.2 + radius * Kore::cos(angle), 0.3 + radius * Kore::sin(angle), 0.3);
		vec4 finalPos = T * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(53, finalPos);	// Left foot 49, right foot 53
		
		// Set hand position
		desPosition = vec3(0.3 + radius * Kore::cos(angle), 1.0 + radius * Kore::sin(angle), 0.3);
		//desPosition = vec3(0.4 + radius * Kore::cos(angle), 1.1, 0.3);
		//desPosition = vec3(0.4, 1.1, 0.3);
		
		finalPos = T * vec4(desPosition.x(), desPosition.y(), desPosition.z(), 1);
		avatar->setDesiredPosition(targetBoneIndex, finalPos);
		
		//mat4 rot = desRotation.matrix().Transpose() * mat4::Rotation(0, 0, 0.01);
		//RotationUtility::getOrientation(&rot, &desRotation);
		
		Kore::mat4 rot_err = initRot * desRotation.matrix().Transpose().Invert();
		//Kore::mat4 rot_err = desRotation.matrix().Transpose() * mat4::Rotation(0, Kore::pi, 0);
		Kore::Quaternion finalRotation;
		RotationUtility::getOrientation(&rot_err, &finalRotation);
		
		//avatar->setLocalRotation(targetBoneIndex-1, Quaternion(0.2, 0.8, 0.0, 1.0)); // lowerarm
		//avatar->setLocalRotation(targetBoneIndex-2, Quaternion(0.1, 0.0, -0.3, 1.0)); // upperarm
		
		//cube->drawVertices(cube->M, V, P, width, height);
		//avatar->drawJoints(avatar->M, V, P, width, height, true);
		
		renderTracker();
		Graphics4::setPipeline(pipeline);
#endif


		Graphics4::end();
		Graphics4::swapBuffers();
	}
	
	void keyDown(KeyCode code) {
		switch (code) {
			case Kore::KeyLeft:
			case Kore::KeyA:
				left = true;
				break;
			case Kore::KeyRight:
			case Kore::KeyD:
				right = true;
				break;
			case Kore::KeyDown:
				down = true;
				break;
			case Kore::KeyUp:
				up = true;
				break;
			case Kore::KeyW:
				forward = true;
				break;
			case Kore::KeyS:
				backward = true;
				break;
			case Kore::KeyX:
				rotateX = true;
				rotateCubeX = true;
				break;
			case Kore::KeyY:
				rotateY = true;
				rotateCubeY = true;
				break;
			case Kore::KeyZ:
				rotateZ = true;
				rotateCubeZ = true;
				break;
			case Kore::KeyR:
#ifdef KORE_STEAMVR
				VrInterface::resetHmdPose();
#endif
				break;
			case KeyL:
				Kore::log(Kore::LogLevel::Info, "Position: (%f, %f, %f)", playerPosition.x(), playerPosition.y(), playerPosition.z());
				Kore::log(Kore::LogLevel::Info, "Rotation: (%f, %f, %f)", globe.x(), globe.y(), globe.z());
				break;
			case KeyQ:
				System::stop();
				break;
			case KeyC:
				rotateCube = true;
				break;
			default:
				break;
		}
	}
	
	void keyUp(KeyCode code) {
		switch (code) {
			case Kore::KeyLeft:
			case Kore::KeyA:
				left = false;
				break;
			case Kore::KeyRight:
			case Kore::KeyD:
				right = false;
				break;
			case Kore::KeyDown:
				down = false;
				break;
			case Kore::KeyUp:
				up = false;
				break;
			case Kore::KeyW:
				forward = false;
				break;
			case Kore::KeyS:
				backward = false;
				break;
			case Kore::KeyX:
				rotateX = false;
				rotateCubeX = false;
				break;
			case Kore::KeyY:
				rotateY = false;
				rotateCubeY = false;
				break;
			case Kore::KeyZ:
				rotateZ = false;
				rotateCubeZ = false;
				break;
			case KeyC:
				rotateCube = false;
				break;
			default:
				break;
		}
	}
	
	void mouseMove(int windowId, int x, int y, int movementX, int movementY) {
		float rotationSpeed = 0.01;
		
		if (rotateX) {
			globe.x() += (float)((mousePressX - x) * rotationSpeed);
			mousePressX = x;
		} else if (rotateZ) {
			globe.z() += (float)((mousePressY - y) * rotationSpeed);
			mousePressY = y;
		}
		
	}
	
	void mousePress(int windowId, int button, int x, int y) {
		//rotateX = true;
		//rotateZ = true;
		mousePressX = x;
		mousePressY = y;
	}
	
	void mouseRelease(int windowId, int button, int x, int y) {
		//rotateX = false;
		//rotateZ = false;
	}
	
	void init() {
		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		//FileReader vs("shader_lighting.vert");
		//FileReader fs("shader_lighting.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
		
		// This defines the structure of your Vertex Buffer
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);
		
		pipeline = new PipelineState;
		pipeline->inputLayout[0] = &structure;
		pipeline->inputLayout[1] = nullptr;
		pipeline->vertexShader = vertexShader;
		pipeline->fragmentShader = fragmentShader;
		pipeline->depthMode = ZCompareLess;
		pipeline->depthWrite = true;
		pipeline->blendSource = Graphics4::SourceAlpha;
		pipeline->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline->compile();

		pipeline2 = new PipelineState;
		pipeline2->inputLayout[0] = &structure;
		pipeline2->inputLayout[1] = nullptr;
		pipeline2->vertexShader = vertexShader;
		pipeline2->fragmentShader = fragmentShader;
		pipeline2->depthMode = ZCompareAlways;
		pipeline2->depthWrite = false;
		pipeline2->blendSource = Graphics4::SourceAlpha;
		pipeline2->blendDestination = Graphics4::InverseSourceAlpha;
		pipeline2->alphaBlendSource = Graphics4::SourceAlpha;
		pipeline2->alphaBlendDestination = Graphics4::InverseSourceAlpha;
		pipeline2->compile();
		
		tex = pipeline->getTextureUnit("tex");
		
		pLocation = pipeline->getConstantLocation("P");
		vLocation = pipeline->getConstantLocation("V");
		mLocation = pipeline->getConstantLocation("M");
		
		cube = new MeshObject("cube.ogex", "", structure, 0.05);
#ifdef KORE_STEAMVR
		avatar = new MeshObject("avatar/avatar_skeleton_headless.ogex", "avatar/", structure);
#else
		avatar = new MeshObject("avatar/avatar_skeleton.ogex", "avatar/", structure);
#endif
		initRot = mat4::RotationX(-Kore::pi / 2.0);
		initRotInv = initRot.Invert();
		
		Graphics4::setTextureAddressing(tex, Graphics4::U, Repeat);
		Graphics4::setTextureAddressing(tex, Graphics4::V, Repeat);
		
#ifdef KORE_STEAMVR
		VrInterface::init(nullptr, nullptr, nullptr); // TODO: Remove
#endif
	}
	
}

int kore(int argc, char** argv) {
	System::init("BodyTracking", width, height);
	
	init();
	
	System::setCallback(update);
	
	startTime = System::time();
	
	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;
	
	System::start();
	
	return 0;
}
