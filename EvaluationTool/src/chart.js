import React from 'react';
import { ResponsiveLine } from '@nivo/line';
import { compose, withPropsOnChange } from 'recompose';
import { get } from 'lodash';
import { colorsMaterial } from '@filou/core';

const enhance = compose(
  withPropsOnChange(['files'], ({ files }) => ({
    large: files.length > 1
  })),
  withPropsOnChange(
    ['files', 'fields', 'groupBy'],
    ({ files, fields, groupBy, large }) => {
      const data = [];

      files.forEach((file, i) => {
        fields.forEach((field, j) => {
          data.push({
            id: !large
              ? field
              : `#${files.findIndex(x => file.name === x.name) +
                  1} ${groupBy}: ${file[groupBy]}`,
            data: file.values[field].map((y, x) => ({ x, y })),
            color: get(colorsMaterial, [i || j, 'palette', 6], 'black')
          });
        });
      });

      return { data };
    }
  ),
  withPropsOnChange(['fields', 'data'], ({ fields, data }) => ({
    // scale values from 0% to 100%
    data:
      fields.length === 1
        ? data
        : data.map(d => {
            const ys = d.data.map(({ y }) => y);
            const lowest = Math.min(...ys);
            const highest = Math.max(...ys) - lowest;

            return {
              ...d,
              data: d.data.map(({ x, y }) => ({
                x,
                y: highest
                  ? Math.floor(((y - lowest) / highest) * 10000) / 100
                  : x
              }))
            };
          })
  })),
  withPropsOnChange(['files', 'data'], ({ files, data }) => {
    if (files.length === 1) return {};

    // reduce all data to consistent length
    let min;
    data.forEach(d => {
      const length = get(d, ['data', 'length']);
      if (!min || length < min) min = length;
    });

    return { data: data.map(d => ({ ...d, data: d.data.slice(0, min) })) };
  })
);

const Chart = ({ data, large }) => (
  <div style={{ flexGrow: 1 }}>
    {!!data.length && (
      <ResponsiveLine
        data={data}
        margin={{
          top: 20,
          right: !large ? 125 : 200,
          bottom: 50,
          left: 50
        }}
        // minY="auto"
        dotSize={10}
        enableDotLabel
        animate
        colorBy={e => e.color}
        legends={[
          {
            anchor: 'bottom-right',
            direction: 'column',
            translateX: !large ? 95 : 170,
            itemWidth: !large ? 75 : 150,
            itemHeight: 20
          }
        ]}
      />
    )}
  </div>
);

export default enhance(Chart);
