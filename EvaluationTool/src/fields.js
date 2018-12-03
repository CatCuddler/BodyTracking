import React from "react";
import { Dropdown, Label } from "semantic-ui-react";
import { get } from "lodash";
import colorsMaterial from "./colors";

const Fields = ({
  groupBy,
  selectedFiles,
  fields,
  selectedFields,
  onClickField
}) => {
  const ffields = fields.filter(
    field => !field.includes(" Min") && !field.includes(" Max")
  );

  return (
    <Dropdown item text="fields" disabled={!ffields.length}>
      <Dropdown.Menu>
        {ffields.map(field => (
          <Dropdown.Item
            key={field}
            active={selectedFields.includes(field)}
            onClick={e => onClickField(e, field)}
            text={field}
            image={
              selectedFields.includes(field) && (
                <Label
                  circular
                  empty
                  style={{
                    backgroundColor:
                      (selectedFiles.length === 1 || !!groupBy) &&
                      get(colorsMaterial, [
                        selectedFields.findIndex(x => x === field),
                        "palette",
                        6
                      ])
                  }}
                />
              )
            }
          />
        ))}
      </Dropdown.Menu>
    </Dropdown>
  );
};

export default Fields;
