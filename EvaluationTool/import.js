const glob = require('glob');
const csvjson = require('csvjson');
const { writeJsonSync, readFileSync, ensureDirSync } = require('fs-extra');
const { resolve } = require('path');
const { input, output, groupByFiles } = require('./config');

const parse = csv =>
  csvjson.toObject(csv, {
    delimiter: ';'
  });

const work = (fromFolder, toFolder) => {
  const files = glob.sync(resolve(fromFolder, 'evaluation*.csv'));
  const data = {};
  const config = {};

  files.forEach(file => {
    const csv = readFileSync(file, { encoding: 'utf8' });
    const fileName = file
      .substr(fromFolder.length + 1)
      .split('.csv')
      .join('')
      .split('_');
    if (fileName[0] === 'evaluationData') data[fileName[1]] = parse(csv);
    else [config[fileName[1]]] = parse(csv);
  });

  Object.keys(data).forEach(file => {
    const folder = groupByFiles
      ? `${config[file].File.split('-')[0]}_${toFolder}`
      : toFolder;

    if (config[file] && config[file]['IK Mode'] < 6) {
      ensureDirSync(resolve(__dirname, 'json', folder));
      writeJsonSync(
        resolve(__dirname, 'json', folder, `evaluationData_${file}.json`),
        data[file]
      );
      writeJsonSync(
        resolve(__dirname, 'json', folder, `evaluationConfig_${file}.json`),
        config[file]
      );
    }
  });

  console.log(`${files.length / 2} Files imported!`);
};

work(resolve(__dirname, input), output);
