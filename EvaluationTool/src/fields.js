import React from 'react';
import { Menu, Label } from 'semantic-ui-react';
import { colorsMaterial } from '@filou/core';
import { get } from 'lodash';

const Fields = ({ fields, selectedFields, onClickField }) => (
  <Menu.Menu position="right">
    {fields.map(field => (
      <Menu.Item
        key={field}
        active={selectedFields.includes(field)}
        onClick={e => onClickField(e, field)}
      >
        {field}
        {selectedFields.includes(field) && (
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
  </Menu.Menu>
);

export default Fields;
