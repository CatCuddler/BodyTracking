import React from 'react';
import { Menu, Label } from 'semantic-ui-react';
import { get } from 'lodash';
import colorsMaterial from './colors';

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
