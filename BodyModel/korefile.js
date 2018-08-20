var project = new Project('BodyTracking', __dirname);

project.addFile('Sources/**');
project.setDebugDir('Deployment');

project.addIncludeDir('../HMM_Trainer/HMM_Trainer');

Project.createProject('Kore', __dirname).then((subproject) => {
	project.addSubProject(subproject);
	resolve(project);
});
