import React, { Fragment } from 'react';
import { Menu, Label } from 'semantic-ui-react';
import { compose, withPropsOnChange } from 'recompose';
import { get, groupBy as _groupBy, sortBy as _sortBy } from 'lodash';
import colorsMaterial from './colors';
import LabelGroup from './labelgroup';
import ikMode from './ik-modes';

const enhance = compose(
  withPropsOnChange(['sortBy'], ({ sortBy }) => ({
    sortBy2: sortBy === 'mode' ? 'modeNumber' : sortBy
  })),
  withPropsOnChange(['sortBy2', 'files'], ({ sortBy2, files }) => ({
    groups: sortBy2 ? _groupBy(files, sortBy2) : { All: files }
  })),
  withPropsOnChange(['sortBy', 'groups'], ({ sortBy, groups }) => {
    const sortedGroups = {};
    _sortBy(Object.keys(groups)).forEach(group => {
      sortedGroups[sortBy === 'mode' ? ikMode[group] || 'All' : group] =
        groups[group];
    });

    return {
      groups: sortedGroups
    };
  })
);

const Files = ({
  selectedFiles,
  groups,
  sortBy,
  groupBy,
  onClickFile,
  onClickFiles
}) => (
  <Menu vertical style={{ width: '25%', overflowY: 'auto', marginBottom: 0 }}>
    {Object.keys(groups).map(group => (
      <Fragment key={group}>
        <Menu.Item
          header
          onClick={e => onClickFiles(e, groups[group].map(file => file.name))}
          active={groups[group].every(
            ({ name }) => selectedFiles.findIndex(file => file === name) >= 0
          )}
        >
          {group}
          {!groupBy ? (
            <Label>{sortBy}</Label>
          ) : (
            <LabelGroup
              length={groups[group].reduce(
                (acc, val) =>
                  acc +
                  get(val, [
                    'values',
                    get(Object.keys(val.values), 0),
                    'length'
                  ]),
                0
              )}
              file={
                (groups[group].every(
                  x => x.file === get(groups, [group, 0, 'file'])
                ) &&
                  get(groups, [group, 0, 'file'])) ||
                '-'
              }
              errorPos={
                (groups[group].every(
                  x => x.errorPos === get(groups, [group, 0, 'errorPos'])
                ) &&
                  get(groups, [group, 0, 'errorPos'])) ||
                '-'
              }
              errorRot={
                (groups[group].every(
                  x => x.errorRot === get(groups, [group, 0, 'errorRot'])
                ) &&
                  get(groups, [group, 0, 'errorRot'])) ||
                '-'
              }
              lambda={
                (groups[group].every(
                  x => x.lambda === get(groups, [group, 0, 'lambda'])
                ) &&
                  get(groups, [group, 0, 'lambda'])) ||
                '-'
              }
              steps={
                (groups[group].every(
                  x => x.steps === get(groups, [group, 0, 'steps'])
                ) &&
                  get(groups, [group, 0, 'steps'])) ||
                '-'
              }
              selectedFiles={selectedFiles}
              groupBy={groupBy}
            />
          )}
        </Menu.Item>
        {!groupBy &&
          groups[group].map(file => {
            const active = selectedFiles.includes(file.name);

            return (
              <Menu.Item
                key={file.name}
                active={active}
                onClick={e => onClickFile(e, file.name)}
                style={{
                  backgroundColor:
                    active &&
                    selectedFiles.length > 1 &&
                    get(colorsMaterial, [
                      selectedFiles.findIndex(x => x === file.name),
                      'palette',
                      6
                    ]),
                  color: active && selectedFiles.length > 1 && 'white'
                }}
              >
                <b>{file.mode}</b> {file.orientation && 'with Orientation'}
                <LabelGroup
                  {...file}
                  selectedFiles={selectedFiles}
                  groupBy={groupBy}
                />
              </Menu.Item>
            );
          })}
      </Fragment>
    ))}
  </Menu>
);

export default enhance(Files);
