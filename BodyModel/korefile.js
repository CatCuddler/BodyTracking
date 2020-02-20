var project = new Project('BodyTracking', __dirname);

project.addFile('Sources/**');
project.setDebugDir('Deployment');

//Needed for JDK
project.addIncludeDir('../zulu13.29.9-ca-jdk13.0.2-win_i686/include');
project.addIncludeDir('../zulu13.29.9-ca-jdk13.0.2-win_i686/include/win32');
project.addLib('../zulu13.29.9-ca-jdk13.0.2-win_i686/lib/jvm');

resolve(project);
