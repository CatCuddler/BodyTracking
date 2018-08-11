import React from 'react';
import { Label } from 'semantic-ui-react';
import { get } from 'lodash';
import { colorsMaterial } from '@filou/core';

const LabelGroup = ({
  name,
  length,
  file,
  errorPos,
  errorRot,
  lambda,
  steps,
  usingClampMag,
  dMaxPos,
  dMaxRot,
  selectedFiles,
  groupBy
}) => (
  <div style={{ marginTop: '0.5rem' }}>
    <Label.Group size="small">
      <Label
        horizontal
        style={
          name
            ? {
                color:
                  !groupBy &&
                  selectedFiles.findIndex(x => x === name) >= 0 &&
                  'white',
                backgroundColor:
                  !groupBy &&
                  get(colorsMaterial, [
                    selectedFiles.findIndex(x => x === name) * 2,
                    'palette',
                    6
                  ])
              }
            : {}
        }
      >
        {name && selectedFiles.length > 1
          ? `#${selectedFiles.findIndex(x => x === name) + 1}`
          : length}
      </Label>
      <Label color="teal" horizontal>
        {file || '-'}
      </Label>
      <Label color="blue" horizontal>
        {lambda || '-'}
      </Label>
      <Label color="red" horizontal>
        {errorPos || '-'} / {errorRot || '-'}
      </Label>
      <Label color="green" horizontal>
        {steps || '-'}
      </Label>
      {usingClampMag && (
        <Label color="violet" horizontal>
          {dMaxPos || '-'} / {dMaxRot || '-'}
        </Label>
      )}
    </Label.Group>
  </div>
);

export default LabelGroup;
