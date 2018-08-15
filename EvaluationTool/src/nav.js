import React from 'react';
import { Menu, Dropdown, Checkbox } from 'semantic-ui-react';
import Fields from './fields';
import Select from './select';

const Nav = ({
  selectedFiles,
  fields,
  selectedFields,
  onClickField,
  folders,
  folder,
  setFolder,
  sortBy,
  setSortBy,
  groupBy,
  setGroupBy,
  min,
  setMin,
  max,
  setMax,
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

    <Select value={sortBy} setValue={setSortBy} />

    <Select value={groupBy} setValue={setGroupBy} allowNull />

    <Menu.Item>
      <Checkbox
        checked={scale}
        onClick={() => setScale(!scale)}
        label="scale"
      />
    </Menu.Item>
    <Menu.Item>
      <Checkbox checked={min} onClick={() => setMin(!min)} label="min" />
    </Menu.Item>
    <Menu.Item>
      <Checkbox checked={max} onClick={() => setMax(!max)} label="max" />
    </Menu.Item>

    <Fields
      groupBy={groupBy}
      selectedFiles={selectedFiles}
      fields={fields}
      selectedFields={selectedFields}
      onClickField={onClickField}
    />
  </Menu>
);

export default Nav;
