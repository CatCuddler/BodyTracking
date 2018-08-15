import React from 'react';
import { ResponsiveLine } from '@nivo/line';
import { compose, withPropsOnChange } from 'recompose';
import { get, groupBy as _groupBy, sortBy as _sortBy } from 'lodash';
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
  withPropsOnChange(['selectedFiles'], ({ selectedFiles }) => ({
    large: selectedFiles.length > 1
  })),
  withPropsOnChange(
    [
      'files',
      'selectedFiles',
      'selectedFields',
      'sortBy',
      'groupBy',
      'min',
      'max'
    ],
    ({
      files,
      selectedFiles,
      selectedFields,
      sortBy,
      groupBy,
      min,
      max,
      large
    }) => {
      const filteredFiles = files.filter(file =>
        selectedFiles.includes(file.name)
      );

      // group multiple files to one
      if (groupBy) {
        const data = [];
        const groups = _groupBy(filteredFiles, sortBy);

        const extendedFields = [];
        selectedFields.forEach(field => {
          extendedFields.push(field);

          if (min && get(selectedFiles, [0, 'values', `${field} Min`], []))
            extendedFields.push(`${field} Min`);
          if (max && get(selectedFiles, [0, 'values', `${field} Max`], []))
            extendedFields.push(`${field} Max`);
        });

        Object.keys(groups).forEach((group, index) => {
          const xData = {};
          const yData = {};

          groups[group].forEach(file => {
            extendedFields.forEach(field => {
              if (!xData[field]) xData[field] = [];
              if (!yData[field]) yData[field] = [];

              xData[field].push(file[groupBy]);
              yData[field].push(
                get(file, ['values', field], []).reduce(
                  (p, c, i, a) => p + c / a.length,
                  0
                )
              );
            });
          });

          Object.keys(xData).forEach(field => {
            const groupNames = Object.keys(groups);
            let chartData = xData[field].map((x, j) => ({
              x: x.toString(),
              y: Math.floor(yData[field][j] * 1000) / 1000
            }));
            chartData = averageDuplicates(chartData);
            chartData = _sortBy(chartData, d => d.x);

            const color =
              groupNames.length > 1
                ? index
                : Object.keys(xData).findIndex(
                    x => x === field.replace(' Min', '').replace(' Max', '')
                  );
            const palette =
              (min && field.includes(' Min')) || (max && field.includes(' Max'))
                ? 1
                : 6;

            data.push({
              id:
                groupNames.length > 1
                  ? `${field} - [${sortBy}: ${group}]`
                  : field,
              data: chartData,
              color: get(
                colorsMaterial,
                [color * 2, 'palette', palette],
                'black'
              )
            });
          });
        });

        return { data };
      }

      // generate min/normal/max-charts
      const data = [];
      filteredFiles.forEach(file => {
        selectedFields.forEach((field, j) => {
          const index = selectedFiles.findIndex(
            fileName => file.name === fileName
          );
          const minValues = min
            ? get(file, ['values', `${field} Min`], [])
            : [];
          const maxValues = max
            ? get(file, ['values', `${field} Max`], [])
            : [];

          if (minValues.length)
            data.push({
              id: !large
                ? `${field} Min`
                : `${field} - #${index + 1} [${sortBy}: ${file[sortBy]}] Min`,
              data: minValues.map((y, x) => ({
                x: x + 1,
                y: parseFloat(y)
              })),
              color: get(
                colorsMaterial,
                [(selectedFiles.length === 1 ? j : index) * 2, 'palette', 1],
                'black'
              )
            });
          if (maxValues.length)
            data.push({
              id: !large
                ? `${field} Max`
                : `${field} - #${index + 1} [${sortBy}: ${file[sortBy]}] Max`,
              data: maxValues.map((y, x) => ({
                x: x + 1,
                y: parseFloat(y)
              })),
              color: get(
                colorsMaterial,
                [(selectedFiles.length === 1 ? j : index) * 2, 'palette', 1],
                'black'
              )
            });

          data.push({
            id: !large
              ? field
              : `${field} - #${index + 1} [${sortBy}: ${file[sortBy]}]`,
            data: get(file, ['values', field], []).map((y, x) => ({
              x: x + 1,
              y: parseFloat(y)
            })),
            color: get(
              colorsMaterial,
              [(selectedFiles.length === 1 ? j : index) * 2, 'palette', 6],
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
  withPropsOnChange(
    ['files', 'data', 'groupBy'],
    ({ files, data, groupBy }) => {
      if (files.length === 1 || groupBy) return {};

      // reduce all data to consistent length
      let min;
      data.forEach(d => {
        const length = get(d, ['data', 'length']);
        if (!min || length < min) min = length;
      });

      return { data: data.map(d => ({ ...d, data: d.data.slice(0, min) })) };
    }
  )
);

const Chart = ({ data, groupBy, large }) => (
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
          right: !large ? 150 : 250,
          bottom: 25,
          left: 50
        }}
        // minY="auto"
        dotSize={10}
        enableDotLabel
        enableArea={!!groupBy}
        animate
        colorBy={e => e.color}
        legends={[
          {
            anchor: 'bottom-right',
            direction: 'column',
            translateX: !large ? 120 : 220,
            itemWidth: !large ? 100 : 200,
            itemHeight: 20
          }
        ]}
      />
    )}
  </div>
);

export default enhance(Chart);
