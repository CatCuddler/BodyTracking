import React from 'react';
import { Menu, Label, Dropdown, Checkbox } from 'semantic-ui-react';
import Fields from './fields';

const Nav = ({
  fields,
  selectedFields,
  onClickField,
  folders,
  folder,
  setFolder,
  groupBy,
  setGroupBy,
  acc,
  setAcc,
  scale,
  setScale
}) => (
  <Menu>
    <Menu.Item header>IK Evaluation Tool</Menu.Item>

    <Dropdown item text={`/${folder}`}>
      <Dropdown.Menu>
        {folders.map(x => (
          <Dropdown.Item
            key={x}
            icon={x === folder ? 'folder open outline' : 'folder outline'}
            text={`/${x}`}
            active={x === folder}
            onClick={() => setFolder(x)}
          />
        ))}
      </Dropdown.Menu>
    </Dropdown>

    <Dropdown item text={groupBy}>
      <Dropdown.Menu>
        <Dropdown.Item
          onClick={() => setGroupBy('mode')}
          active={groupBy === 'mode'}
        >
          <Label circular empty />
          mode
        </Dropdown.Item>
        <Dropdown.Item
          onClick={() => setGroupBy('orientation')}
          active={groupBy === 'orientation'}
        >
          <Label circular empty />
          orientation
        </Dropdown.Item>
        <Dropdown.Item
          onClick={() => setGroupBy('file')}
          active={groupBy === 'file'}
        >
          <Label circular empty />
          file
        </Dropdown.Item>
        <Dropdown.Item
          onClick={() => setGroupBy('lambda')}
          active={groupBy === 'lambda'}
        >
          <Label color="blue" circular empty />
          lambda
        </Dropdown.Item>
        <Dropdown.Item
          onClick={() => setGroupBy('error')}
          active={groupBy === 'error'}
        >
          <Label color="red" circular empty />
          error
        </Dropdown.Item>
        <Dropdown.Item
          onClick={() => setGroupBy('steps')}
          active={groupBy === 'steps'}
        >
          <Label color="green" circular empty />
          steps
        </Dropdown.Item>
      </Dropdown.Menu>
    </Dropdown>

    <Menu.Item>
      <Checkbox checked={acc} onClick={() => setAcc(!acc)} label="merge" />
    </Menu.Item>
    <Menu.Item>
      <Checkbox
        checked={scale}
        onClick={() => setScale(!scale)}
        label="scale"
      />
    </Menu.Item>

    <Fields
      fields={fields}
      selectedFields={selectedFields}
      onClickField={onClickField}
    />
  </Menu>
);

export default Nav;
