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
  acc,
  selectedFiles,
  groups,
  groupBy,
  onClickFile,
  onClickFiles
}) => (
  <Menu vertical style={{ width: '20%', overflowY: 'auto', marginBottom: 0 }}>
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
                  !acc &&
                  active &&
                  selectedFiles.length > 1 &&
                  get(colorsMaterial, [
                    selectedFiles.findIndex(x => x === file.name) * 2,
                    'palette',
                    6
                  ]),
                color: !acc && active && selectedFiles.length > 1 && 'white'
              }}
            >
              <b>{file.mode}</b> {file.orientation && 'with Orientation'}
              <div style={{ marginTop: '0.5rem' }}>
                <Label.Group size="small">
                  <Label
                    horizontal
                    style={{
                      color:
                        !acc &&
                        selectedFiles.findIndex(x => x === file.name) >= 0 &&
                        'white',
                      backgroundColor:
                        !acc &&
                        get(colorsMaterial, [
                          selectedFiles.findIndex(x => x === file.name) * 2,
                          'palette',
                          6
                        ])
                    }}
                  >
                    {selectedFiles.length > 1
                      ? `#${selectedFiles.findIndex(x => x === file.name) + 1}`
                      : file.length}
                  </Label>
                  <Label color="teal" horizontal>
                    {file.file || '-'}
                  </Label>
                  <Label color="blue" horizontal>
                    {file.lambda || '-'}
                  </Label>
                  <Label color="red" horizontal>
                    {file.errorPos || '-'} / {file.errorRot || '-'}
                  </Label>
                  <Label color="green" horizontal>
                    {file.steps || '-'}
                  </Label>
                </Label.Group>
              </div>
            </Menu.Item>
          );
        })}
      </Fragment>
    ))}
  </Menu>
);

export default enhance(Files);
