# Real-time body tracking in virtual reality using a Vive Tracker

This project implements an accurate, low-latency body tracking approach for VR-based applications using Vive Trackers. By implementing an inverse kinematics solver, we can provide appropriate set of joint configurations in order to animate the motions of the avatar as smoothly, rapidly and as accurately as possible. Using a HTC Vive headset, two Vive Controllers and three Vive Trackers, attached to the hands and feet, it is possible to create immersive VR experiences, where the user is able to perceive the avatar as her/his own body.

Requirements:
- node.js
- VisualStudio or XCode

git clone --recursive git@github.com:CatCuddler/BodyTracking.git
cd BodyTracking/BodyModel

Compile for VR:	node Kore/make --vr steamvr
Compile without VR node Kore/make

Currently Metal does not work. Use Opengl or Direct3D.
-g opengl or -g direct3d11

If you are using Xcode, you have to set custom working directory:
BodyTracking -> Edite Scheme... -> Run -> Options and then set custom working directory to 'Deployment'

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/F6yFdpnhhoo/0.jpg)](https://www.youtube.com/watch?v=F6yFdpnhhoo&feature=youtu.be)
