import React from 'react';
import { Menu, Label } from 'semantic-ui-react';
import { colorsMaterial } from '@filou/core';
import { get } from 'lodash';

const Fields = ({
  groupBy,
  selectedFiles,
  fields,
  selectedFields,
  onClickField
}) => (
  <Menu.Menu position="right">
    {fields
      .filter(field => !field.includes(' Min') && !field.includes(' Max'))
      .map(field => (
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
                backgroundColor:
                  (selectedFiles.length === 1 || !!groupBy) &&
                  get(colorsMaterial, [
                    selectedFields.findIndex(x => x === field) * 2,
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
