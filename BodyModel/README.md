# Real-time body tracking in virtual reality using a Vive Tracker

This project implements an accurate, low-latency body tracking approach for VR-based applications using Vive Trackers. By implementing an inverse kinematics solver, we can provide appropriate set of joint configurations in order to animate the motions of the avatar as smoothly, rapidly and as accurately as possible. Using a HTC Vive headset and Vive Tracker, attached to the hands and feet, it is possible to create immersive VR experiences, where the user is able to perceive the avatar as her/his own body.

Compile for VR:
node Kore/make --vr steamvr

If you are using Xcode, you have to set custom working directory:
BodyTracking -> Edite Scheme... -> Run -> Options and then set custom working directory to 'Deployment'

[![IMAGE ALT TEXT HERE](http://img.youtube.com/vi/YOUTUBE_VIDEO_ID_HERE/0.jpg)](https://www.youtube.com/watch?v=F6yFdpnhhoo&feature=youtu.be)
