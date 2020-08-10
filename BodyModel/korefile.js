var project = new Project('BodyTracking', __dirname);

project.addFile('Sources/**');
project.setDebugDir('Deployment');

// Needed for Hidden Markov Models
project.addIncludeDir('../HMM_Trainer/Sources');
project.addFile('../HMM_Trainer/Sources/kMeans.cpp');
project.addFile('../HMM_Trainer/Sources/kMeans.h');
project.addFile('../HMM_Trainer/Sources/Markov.cpp');
project.addFile('../HMM_Trainer/Sources/Markov.h');

// Needed for motion recognition with Weka
project.addIncludeDir('../zulu14.29.23-ca-jdk14.0.2-win_x64/include');
project.addIncludeDir('../zulu14.29.23-ca-jdk14.0.2-win_x64/include/win32');
project.addLib('../zulu14.29.23-ca-jdk14.0.2-win_x64/lib/jvm');


resolve(project);
