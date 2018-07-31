import React, { Fragment } from 'react';
import { ResponsiveLine } from '@nivo/line';
import { Menu, Label } from 'semantic-ui-react';
import { compose, withState, withPropsOnChange, withHandlers } from 'recompose';
import { get } from 'lodash';
import { colorsMaterial } from '@filou/core';

const ikMode = ['JT', 'JPI', 'DLS', 'SVD', 'DLS-SVD', 'SDLS'];
const group = {
  'IK Mode': 'IK',
  'with Orientation': 'orientation',
  File: 'file',
  lambda: <span>&lambda;</span>,
  'Error Max': 'error',
  'Steps Max': 'steps',
};

const getFileName = file =>
  get(get(file.split('positionData_'), 1, '').split('.csv'), 0, '-');

const sources = {};
let configs = {};
const req = require.context('../json');
req.keys().forEach(file => {
  if (!file.includes('.json')) {
    if (file.includes('evaluationData_')) sources[file] = req(file);
    else if (file.includes('evaluationConfig_'))
      configs[file.split('evaluationConfig_').join('evaluationData_')] = req(
        file
      ).map(x => ({
        ...x,
        lambda: Math.floor(x['lambda'] * 10000) / 10000,
        File: getFileName(x['File']),
        'with Orientation': x['with Orientation'] !== '0',
        'IK Mode': ikMode[x['IK Mode']] || 'NA',
      }));
  }
});

const enhance = compose(
  withState('groupBy', 'setGroupBy', 'IK Mode'),
  withState('files', 'setFiles', [get(Object.keys(sources), 0)]),
  withPropsOnChange(['files'], ({ files }) => ({
    keys: Object.keys(get(sources, [get(files, 0), 0], {})),
  })),
  withState('values', 'setValues', ({ keys }) => keys.filter((x, i) => !i)),
  withPropsOnChange(['values', 'files'], ({ values, files }) => {
    // Werte filtern & ermitteln
    let fields = {};
    files.forEach(file => {
      if (!fields[file]) fields[file] = {};

      get(sources, file, []).forEach(row => {
        Object.keys(row).forEach(value => {
          if (values.includes(value)) {
            if (!fields[file][value]) fields[file][value] = [];

            fields[file][value].push(Math.floor(row[value] * 1000) / 1000);
          }
        });
      });
    });

    return { fields };
  }),
  withPropsOnChange(['values', 'fields'], ({ values, fields }) => {
    // Werte skalieren von 0% bis 100%
    if (values.length > 1)
      Object.keys(fields).forEach(file => {
        Object.keys(fields[file]).forEach(field => {
          const lowest = Math.min(...fields[file][field]);
          const highest = Math.max(...fields[file][field]) - lowest;

          fields[file][field] = fields[file][field].map(
            x =>
              highest ? Math.floor(((x - lowest) / highest) * 10000) / 100 : x
          );
        });
      });

    return { fields };
  }),
  withPropsOnChange(
    ['fields', 'files', 'groupBy'],
    ({ fields, files, groupBy }) => {
      // Werte auf Form für Chart bringen
      let data = [],
        min = 0;
      Object.keys(fields).forEach(file => {
        Object.keys(fields[file]).forEach(id => {
          const config = get(configs, [file, 0], {});

          data.push({
            id:
              files.length === 1
                ? id
                : `#${files.findIndex(x => x === file) + 1} ${
                    config['IK Mode']
                  } ${config['with Orientation'] ? '(orientation)' : ''} ${
                    groupBy !== 'IK Mode' && groupBy !== 'with Orientation'
                      ? `${group[groupBy]}: ${config[groupBy]}`
                      : ''
                  }`,
            data: fields[file][id].map((y, x) => ({ x: x + 1, y })),
          });
        });

        const length = get(data, [data.length - 1, 'data', 'length']);
        if (!min || length < min) min = length;
      });
      data = data.map(({ data: innerData, ...rest }, i) => ({
        data: innerData.slice(0, min),
        color: get(colorsMaterial, [i * 2, 'palette', 6], 'black'),
        ...rest,
      }));

      return {
        data,
      };
    }
  ),
  withHandlers({
    onClickValue: ({ files, values, setValues }) => (e, value) =>
      e.shiftKey && files.length === 1
        ? setValues(
            values.includes(value)
              ? values.filter(x => x !== value)
              : [...values, value]
          )
        : setValues([value]),
    onClickFile: ({ values, setValues, files, setFiles }) => (e, file) => {
      if (e.shiftKey && (files.length > 1 || !files.includes(file))) {
        setFiles(
          files.includes(file)
            ? files.filter(x => x !== file)
            : [...files, file]
        );
        setValues([get(values, 0)]);
      } else setFiles([file]);
    },
  }),
  withPropsOnChange(['groupBy', 'files'], ({ groupBy, files, onClickFile }) => {
    const items = {};
    Object.keys(sources).forEach(file => {
      const config = get(configs, [file, 0], {});
      const mode = config[groupBy];
      const index = files.findIndex(x => x === file);

      if (!items[mode]) items[mode] = [];

      items[mode].push(
        <Menu.Item
          key={file}
          active={files.includes(file)}
          onClick={e => onClickFile(e, file)}
        >
          <b>{config['IK Mode']}</b>{' '}
          {config['with Orientation'] && 'with Orientation'}
          <Label
            style={{
              backgroundColor: get(colorsMaterial, [index * 2, 'palette', 6]),
            }}
            circular={!files.includes(file)}
          >
            {files.includes(file) ? `#${index + 1}` : sources[file].length}
          </Label>
          <div style={{ marginTop: '0.25rem', marginLeft: '-0.25rem' }}>
            <Label color="teal" size="small" horizontal>
              {config['File']}
            </Label>
            <Label color="blue" size="small" horizontal>
              {config['lambda']}
            </Label>
            <Label color="red" size="small" horizontal>
              {config['Error Max']}
            </Label>
            <Label color="green" size="small" horizontal>
              {config['Steps Max']}
            </Label>
          </div>
        </Menu.Item>
      );
    });

    return { items };
  })
);

const App = ({
  items,
  data,
  files,
  values,
  keys,
  groupBy,
  setGroupBy,
  onClickValue,
  onClickFile,
}) => {
  const [file = ''] = files;
  const [config] = configs[file] || [{}];

  return (
    <div
      style={{
        display: 'flex',
        flexDirection: 'column',
        height: '100vh',
        padding: '2rem',
      }}
    >
      <Menu>
        <Menu.Item header>IK Evaluation Tool</Menu.Item>
        {keys.map(value => (
          <Menu.Item
            key={value}
            active={values.includes(value)}
            onClick={e => onClickValue(e, value)}
          >
            {value.split('[us]').join('[μs]')}
            {!!values.includes(value) && (
              <Label
                circular
                empty
                style={{
                  backgroundColor: get(colorsMaterial, [
                    values.findIndex(x => x === value) * 2,
                    'palette',
                    6,
                  ]),
                }}
              />
            )}
          </Menu.Item>
        ))}

        <Menu.Menu position="right" disabled>
          <Menu.Item
            onClick={() => setGroupBy('IK Mode')}
            active={groupBy === 'IK Mode'}
          >
            mode{' '}
            {files.length === 1 && <Label floating>{config['IK Mode']}</Label>}
          </Menu.Item>
          <Menu.Item
            onClick={() => setGroupBy('with Orientation')}
            active={groupBy === 'with Orientation'}
          >
            orientation{' '}
            {files.length === 1 && (
              <Label floating>
                {config['with Orientation'] ? 'true' : 'false'}
              </Label>
            )}
          </Menu.Item>
          <Menu.Item
            onClick={() => setGroupBy('File')}
            active={groupBy === 'File'}
          >
            file
            {files.length === 1 && (
              <Label floating color="teal">
                {get(config, 'File', '')}
              </Label>
            )}
          </Menu.Item>
          <Menu.Item
            onClick={() => setGroupBy('lambda')}
            active={groupBy === 'lambda'}
          >
            lambda
            {files.length === 1 &&
              config['lambda'] !== -1 && (
                <Label floating color="blue">
                  {config['lambda']}
                </Label>
              )}
          </Menu.Item>
          <Menu.Item
            onClick={() => setGroupBy('Error Max')}
            active={groupBy === 'Error Max'}
          >
            error
            {files.length === 1 && (
              <Label floating color="red">
                {config['Error Max']}
              </Label>
            )}
          </Menu.Item>
          <Menu.Item
            onClick={() => setGroupBy('Steps Max')}
            active={groupBy === 'Steps Max'}
          >
            steps
            {files.length === 1 && (
              <Label floating color="green">
                {config['Steps Max']}
              </Label>
            )}
          </Menu.Item>
        </Menu.Menu>
      </Menu>

      <div style={{ flexGrow: 1, display: 'flex' }}>
        <Menu vertical style={{ width: '20%', overflowY: 'auto' }}>
          {Object.keys(items).map(key => (
            <Fragment key={key}>
              <Menu.Item header>
                {group[groupBy]}: <u>{key}</u>
              </Menu.Item>
              {items[key]}
            </Fragment>
          ))}
        </Menu>

        <div style={{ flexGrow: 1 }}>
          {!!data.length && (
            <ResponsiveLine
              data={data}
              margin={{
                top: 20,
                right: files.length === 1 ? 125 : 200,
                bottom: 50,
                left: 50,
              }}
              // minY="auto"
              dotSize={10}
              enableDotLabel
              animate
              colorBy={e => e.color}
              legends={[
                {
                  anchor: 'bottom-right',
                  direction: 'column',
                  translateX: files.length === 1 ? 95 : 170,
                  itemWidth: files.length === 1 ? 75 : 150,
                  itemHeight: 20,
                },
              ]}
            />
          )}
        </div>
      </div>
    </div>
  );
};

export default enhance(App);
