const glob = require('glob');
const csvjson = require('csvjson');
const { writeJsonSync, readFileSync, ensureDirSync } = require('fs-extra');
const { resolve } = require('path');
const { input, output } = require('./config');

const parse = csv =>
  csvjson.toObject(csv, {
    delimiter: ';'
  });

const work = (fromFolder, toFolder) => {
  const files = glob.sync(resolve(fromFolder, 'evaluation*.csv'));

  ensureDirSync(toFolder);

  files.forEach(file => {
    const csv = readFileSync(file, { encoding: 'utf8' });
    const fileName = file
      .substr(fromFolder.length + 1)
      .replace('.csv', '.json');

    writeJsonSync(resolve(toFolder, fileName), parse(csv));
  });

  console.log(`${files.length / 2} Files imported!`);
};

work(resolve(__dirname, input), resolve(__dirname, 'json', output));
