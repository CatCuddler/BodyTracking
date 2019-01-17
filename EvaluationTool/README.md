## Installation

- Install node (https://nodejs.org/en/download/)
- Install yarn (https://yarnpkg.com/lang/en/docs/install/)

## Usage

### Log data for evaluation

- Start IK-Project in xcode
- Set eval = true (settings.h) to log IK for evaluating
- Run IK at least once until movement has been completely executed, then the algorithm starts automatically from the beginning
- For every whole iteration one row is added to evaluation file

### Import and analyse files with evaluation tool

- Open Terminal/Console
- Run "node import" to import data from Deployment-folder (you can change it in config.json)
- Eventuelly you have to run "npm install"
- Run "yarn start" or "sudo yarn start"
- The browser will open the tool automatically (otherwise open http://localhost:3000/ in browser)
- Click on a file (left sidebar) and a value (upper navbar) to see the chart
- Press Shift + Click to select multiple Files/Values
- To import new data, simply run "node import" again -> the browser will refresh automatically
- Click on right tabs to group files by ik-mode, orientation, file, ...
- To analyse multiple files, select the checkbox on the right (upper nav)
