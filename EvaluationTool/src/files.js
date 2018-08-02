import React, { Fragment } from 'react';
import { Menu, Label } from 'semantic-ui-react';
import { compose, withPropsOnChange } from 'recompose';
import { get, groupBy as _groupBy, sortBy } from 'lodash';
import { colorsMaterial } from '@filou/core';

const enhance = compose(
  withPropsOnChange(['groupBy', 'files'], ({ groupBy, files }) => ({
    groups: _groupBy(files, groupBy),
    groupBy
  }))
);

const Files = ({
  selectedFiles,
  groups,
  groupBy,
  onClickFile,
  onClickFiles
}) => (
  <Menu vertical style={{ width: '20%', overflowY: 'auto' }}>
    {sortBy(Object.keys(groups)).map(group => (
      <Fragment key={group}>
        <Menu.Item
          header
          onClick={e => onClickFiles(e, groups[group].map(file => file.name))}
        >
          {group}
          <Label>{groupBy}</Label>
        </Menu.Item>
        {groups[group].map(file => {
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
              <Label
                style={{
                  backgroundColor: get(colorsMaterial, [
                    selectedFiles.findIndex(x => x === file.name),
                    'palette',
                    6
                  ])
                }}
                circular
              >
                {selectedFiles.length > 1
                  ? `#${selectedFiles.findIndex(x => x === file.name) + 1}`
                  : file.length}
              </Label>
              <div style={{ marginTop: '0.25rem', marginLeft: '-0.25rem' }}>
                <Label color="teal" size="small" horizontal>
                  {file.file}
                </Label>
                <Label color="blue" size="small" horizontal>
                  {file.lambda}
                </Label>
                <Label color="red" size="small" horizontal>
                  {file.error}
                </Label>
                <Label color="green" size="small" horizontal>
                  {file.steps}
                </Label>
              </div>
            </Menu.Item>
          );
        })}
      </Fragment>
    ))}
  </Menu>
);

export default enhance(Files);
