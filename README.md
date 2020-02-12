# Weka based motion recognition

## Setup

### Java

The Weka machine learning framework and the Weka Helper run within a Java Virtual Machine. Since the body Tracking Framework does not currently feature 64-bit support, live classification requires a 32-bit Java Development Kit. However, Oracle does not provide 32-bit JDKs for Java 9 and upwards. The Body Tracking Framework and Weka Helper have been tested with Zulu 10.3, a 32-bit JDK 10 distribution provided by [Azul](https://www.azul.com/downloads/zulu/).

After installing / unpacking the JDK, add the following environment variables to your system "Path" variable:

- (JDK installation directory) / bin
- (JDK installation directory) / bin / server

Afterwards, restart your computer.

### IDE

- Add (JDK installation directory)\include and (JDK installation directory)\include\win32 to the include directories of your compiler
(for Visual Studio 2017 [15.8.6], right-click on the project in Solution Explorer -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories)
- Add (JDK installation directory)\lib\jvm.lib to the linker input files as additional dependencies
(for Visual Studio 2017 [15.8.6], right-click on the project in Solution Explorer -> Properties -> Configuration Properties -> Linker -> Input -> Additional Dependencies)
- If you experience errors that standard libraries cannot be found (e.g. "cannot open source file math.h), you may have to manually set the "Windows SDK Version" and "Platform Toolset" for your project. 
(for Visual Studio 2017 [15.8.6], right-click on the project in Solution Explorer -> Properties -> Configuration Properties -> General, set "Windows SDK Version" to a specific version available on your system, such as 10.0.17763.0, and do the same for "Platform Toolset", for example v141)


## Usage

### Recording

All settings are made within the MachineLearningMotionRecognition.h file. To record new movement data, first set the "operatingMode" to "RecordMovements". You can also change the name of the recorded subject, the session ID, an optional file tag, and the names of the tasks that you want to record. The recording process is controlled through the following keys:

- [H] : set avatar size 
- [K] : Calibrate controllers and trackers (while subject is in T-Pose)
- [B] : Block all gamepad inputs, to prevent user from accidentally recalibrating (also blocked automatically while recording)
- [Numpad 0 - 9] : Start recording the previously defined tasks
- [Space] : Stop the recording currently in progress

The recorded sensor data for each task is saved in 
(Project Folder) \ MachineLearningMotionRecognition \ Recordings.

### Model training and testing
The [Weka Helper](https://github.com/riruroman/WekaHelper) is compatible with the data produced by the Body Tracking Framework, and can be used to train and evaluate models for different sensor subsets and classifiers.

### Live Classification
The Weka Helper can also be used to predict the currently performed exercise based on the incoming sensor data. After training a model and packaging the Weka Helper according to its documentation, place the jar and model files in (Project Folder) \ MachineLearningMotionRecognition \ Weka. The jar file should be named "WekaMotionRecognitionForCpp-1.0-jar-with-dependencies.jar", and the model "wekaModel.model". Afterwards, set the "operatingMode" within MachineLearningMotionRecognition.h to "RecognizeMovements". 

When running the live classification for the first time, you may encounter an error similar to this one:
	
	Exception thrown at 0x22F004EC in BodyTracking.exe:
	0xC0000005: Access violation reading location 0x00000000.
	
This access violation is intentionally caused, and handled, by the Java Virtual Machine.	It is used to probe	for and deal with a specific erroneous behavior of the operating system. If you do encounter this error, simply allow your IDE to ignore it in the future, and continue the execution.

Once running, the classification process is started by pressing [Space] after the player avatar has been calibrated. Every second, the current prediction will then be logged in the console, and an audio file naming the exercise will be played.

When changing to another model, you need to make sure that the Weka Helper jar was compiled with the same settings. Also note that the "processMovementData" method in MachineLearningMotionRecognition.cpp currently contains a list of allowed sensor positions for the live classifications, which needs to be modified if other sensors are to be used. It currently contains the positions rForeArm, lLeg, lForearm, rLeg, hip, lHand, rHand and head.