import React from 'react';
import { Menu, Label, Dropdown } from 'semantic-ui-react';
import { get } from 'lodash';
import { colorsMaterial } from '@filou/core';

const Nav = ({
  folders,
  folder,
  setFolder,
  files,
  fields,
  selectedFields,
  onClickField,
  groupBy,
  setGroupBy
}) => (
  <Menu>
    <Menu.Item header>IK Evaluation Tool</Menu.Item>
    <Dropdown item text="json" value={folder}>
      <Dropdown.Menu>
        {folders.map(x => (
          <Dropdown.Item key={x} onClick={() => setFolder(x)}>
            {x}
          </Dropdown.Item>
        ))}
      </Dropdown.Menu>
    </Dropdown>

    {fields.map(field => (
      <Menu.Item
        key={field}
        active={selectedFields.includes(field)}
        onClick={e => onClickField(e, field)}
      >
        {field}
        {!!selectedFields.includes(field) && (
          <Label
            circular
            empty
            style={{
              backgroundColor: get(colorsMaterial, [
                selectedFields.findIndex(x => x === field),
                'palette',
                6
              ])
            }}
          />
        )}
      </Menu.Item>
    ))}

    <Menu.Menu position="right" disabled>
      <Menu.Item onClick={() => setGroupBy('mode')} active={groupBy === 'mode'}>
        mode {files.length === 1 && <Label floating>{files[0].mode}</Label>}
      </Menu.Item>
      <Menu.Item
        onClick={() => setGroupBy('orientation')}
        active={groupBy === 'orientation'}
      >
        orientation{' '}
        {files.length === 1 && (
          <Label floating>{files[0].orientation ? 'true' : 'false'}</Label>
        )}
      </Menu.Item>
      <Menu.Item onClick={() => setGroupBy('file')} active={groupBy === 'file'}>
        file
        {files.length === 1 && (
          <Label floating color="teal">
            {files[0].file}
          </Label>
        )}
      </Menu.Item>
      <Menu.Item
        onClick={() => setGroupBy('lambda')}
        active={groupBy === 'lambda'}
      >
        lambda
        {files.length === 1 &&
          files[0].lambda !== -1 && (
            <Label floating color="blue">
              {files[0].lambda}
            </Label>
          )}
      </Menu.Item>
      <Menu.Item
        onClick={() => setGroupBy('error')}
        active={groupBy === 'error'}
      >
        error
        {files.length === 1 && (
          <Label floating color="red">
            {files[0].error}
          </Label>
        )}
      </Menu.Item>
      <Menu.Item
        onClick={() => setGroupBy('steps')}
        active={groupBy === 'steps'}
      >
        steps
        {files.length === 1 && (
          <Label floating color="green">
            {files[0].steps}
          </Label>
        )}
      </Menu.Item>
    </Menu.Menu>
  </Menu>
);

export default Nav;
