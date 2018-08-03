import React from 'react';
import { ResponsiveLine } from '@nivo/line';
import { compose, withPropsOnChange } from 'recompose';
import { get, sortBy } from 'lodash';
import { colorsMaterial } from '@filou/core';

const averageDuplicates = (data, searchIndex = 0, values = []) => {
  // searchIndex is at the end
  if (searchIndex >= data.length) return data;

  // search duplicate
  const index = data.findIndex(
    (d, i) => d.x === data[searchIndex].x && i > searchIndex
  );

  // duplicates found
  if (index >= 0) {
    values.push(data[index].y);

    return averageDuplicates(
      data.filter((d, i) => i !== index),
      searchIndex,
      values
    );
  }

  // no more duplicates found
  values.push(data[searchIndex].y);
  const newData = [...data];
  newData[searchIndex] = {
    x: data[searchIndex].x,
    y:
      Math.floor(
        (values.reduce((acc, value) => acc + value, 0) / values.length) * 1000
      ) / 1000
  };

  return averageDuplicates(newData, searchIndex + 1, []);
};

const enhance = compose(
  withPropsOnChange(['files'], ({ files }) => ({
    large: files.length > 1
  })),
  withPropsOnChange(
    ['files', 'selectedFields', 'groupBy', 'acc'],
    ({ files, selectedFields, groupBy, acc, large }) => {
      if (acc) {
        const xData = {};
        const yData = {};

        files.forEach(file => {
          selectedFields.forEach(field => {
            if (!xData[field]) xData[field] = [];
            if (!yData[field]) yData[field] = [];

            xData[field].push(file[groupBy]);
            yData[field].push(
              file.values[field].reduce((p, c, i, a) => p + c / a.length, 0)
            );
          });
        });

        return {
          data: Object.keys(xData).map((field, i) => {
            let data = xData[field].map((x, j) => ({
              x: x.toString(),
              y: Math.floor(yData[field][j] * 1000) / 1000
            }));
            data = averageDuplicates(data);
            data = sortBy(data, d => d.x);

            return {
              id: field,
              data,
              color: get(colorsMaterial, [i * 2, 'palette', 6], 'black')
            };
          })
        };
      }

      const data = [];
      files.forEach((file, i) => {
        selectedFields.forEach((field, j) => {
          data.push({
            id: !large
              ? field
              : `${field} - #${files.findIndex(x => file.name === x.name) +
                  1} [${groupBy}: ${file[groupBy]}]`,
            data: get(file, ['values', field], []).map((y, x) => ({
              x,
              y: parseFloat(y)
            })),
            color: get(
              colorsMaterial,
              [(files.length === 1 ? j : i) * 2, 'palette', 6],
              'black'
            )
          });
        });
      });

      return { data };
    }
  ),
  withPropsOnChange(['scale', 'data'], ({ scale, data }) => ({
    // scale values from 0% to 100%
    data: !scale
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
                : y
            }))
          };
        })
  })),
  withPropsOnChange(['files', 'data', 'acc'], ({ files, data, acc }) => {
    if (files.length === 1 || acc) return {};

    // reduce all data to consistent length
    let min;
    data.forEach(d => {
      const length = get(d, ['data', 'length']);
      if (!min || length < min) min = length;
    });

    return { data: data.map(d => ({ ...d, data: d.data.slice(0, min) })) };
  })
);

const Chart = ({ data, acc, large }) => (
  <div
    style={{
      flexGrow: 1,
      marginLeft: '1rem',
      display: 'flex',
      flexDirection: 'column'
    }}
  >
    {!!data.length && (
      <ResponsiveLine
        data={data}
        margin={{
          top: 20,
          right: !large ? 125 : 250,
          bottom: 25,
          left: 50
        }}
        // minY="auto"
        dotSize={10}
        enableDotLabel
        enableArea={acc}
        animate
        colorBy={e => e.color}
        legends={[
          {
            anchor: 'bottom-right',
            direction: 'column',
            translateX: !large ? 95 : 220,
            itemWidth: !large ? 75 : 200,
            itemHeight: 20
          }
        ]}
      />
    )}
  </div>
);

export default enhance(Chart);
