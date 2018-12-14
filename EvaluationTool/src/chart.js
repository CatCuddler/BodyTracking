import React from "react";
import { compose, withPropsOnChange, withHandlers } from "recompose";
import { get, groupBy as _groupBy, sortBy as _sortBy } from "lodash";
import { createComponent } from "react-fela";
import { ResponsiveLine } from "./line";
import colorsMaterial from "./colors";

const labelScale = ["", "min-max norm", "z-score norm"];

const Div = createComponent(() => ({
  flexGrow: 1,
  marginLeft: "1rem",
  display: "flex",
  flexDirection: "column",
  "& text": {
    fontSize: "35px !important"
  }
}));

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
    ...data[searchIndex],
    y:
      Math.round(
        (values.reduce((acc, value) => acc + value, 0) / values.length) * 1000
      ) / 1000
  };

  return averageDuplicates(newData, searchIndex + 1, []);
};

const enhance = compose(
  withPropsOnChange(["selectedFiles"], ({ selectedFiles }) => ({
    large: selectedFiles.length > 1
  })),
  withPropsOnChange(
    [
      "files",
      "selectedFiles",
      "selectedFields",
      "sortBy",
      "groupBy",
      "min",
      "max"
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
        const groups = sortBy
          ? _groupBy(filteredFiles, sortBy)
          : { All: files };

        const extendedFields = [];
        selectedFields.forEach(field => {
          extendedFields.push(field);

          if (min && get(selectedFiles, [0, "values", `${field} Min`], []))
            extendedFields.push(`${field} Min`);
          if (max && get(selectedFiles, [0, "values", `${field} Max`], []))
            extendedFields.push(`${field} Max`);
        });

        Object.keys(groups).forEach((group, index) => {
          const xData = {};
          const yData = {};
          const sortData = {};

          groups[group].forEach(file => {
            extendedFields.forEach(field => {
              if (!xData[field]) xData[field] = [];
              if (!yData[field]) yData[field] = [];
              if (!sortData[field]) sortData[field] = [];

              xData[field].push(file[groupBy]);
              yData[field].push(
                get(file, ["values", field], []).reduce(
                  (p, c, i, a) => p + c / a.length,
                  0
                )
              );
              sortData[field].push(
                groupBy === "mode" ? file.modeNumber : file[groupBy]
              );
            });
          });

          Object.keys(xData).forEach(field => {
            const groupNames = Object.keys(groups);
            let chartData = xData[field].map((x, j) => ({
              x,
              y: Math.round(yData[field][j] * 1000) / 1000,
              sort: sortData[field][j]
            }));
            chartData = averageDuplicates(chartData);
            chartData = _sortBy(chartData, d => d.sort);

            const color =
              groupNames.length > 1
                ? index
                : Object.keys(xData).findIndex(
                    x => x === field.replace(" Min", "").replace(" Max", "")
                  );
            const palette =
              (min && field.includes(" Min")) || (max && field.includes(" Max"))
                ? 1
                : 6;

            data.push({
              id: groupNames.length > 1 ? `${field} [${group}]` : field,
              data: chartData,
              color: get(colorsMaterial, [color, "palette", palette], "black")
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
            ? get(file, ["values", `${field} Min`], [])
            : [];
          const maxValues = max
            ? get(file, ["values", `${field} Max`], [])
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
                [selectedFiles.length === 1 ? j : index, "palette", 1],
                "black"
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
                [selectedFiles.length === 1 ? j : index, "palette", 1],
                "black"
              )
            });

          data.push({
            id: !large
              ? field
              : `${field} - #${index + 1} [${sortBy}: ${file[sortBy]}]`,
            data: get(file, ["values", field], []).map((y, x) => ({
              x: x + 1,
              y: parseFloat(y)
            })),
            color: get(
              colorsMaterial,
              [selectedFiles.length === 1 ? j : index, "palette", 6],
              "black"
            )
          });
        });
      });

      return { data };
    }
  ),
  withPropsOnChange(["scale", "data"], ({ scale, data }) => ({
    // scale values from 0% to 100%
    data:
      scale === 1 || scale === -1
        ? data.map(d => {
            const ys = d.data.map(({ y }) => y);
            const lowest = Math.min(...ys);
            const highest = Math.max(...ys) - lowest;

            return {
              ...d,
              data: d.data.map(({ x, y }) => ({
                x,
                y: highest
                  ? Math.round(((y - lowest) / highest) * 100000) / 1000
                  : y
              }))
            };
          })
        : data
  })),
  withPropsOnChange(["files", "data"], ({ files, data }) => {
    if (files.length === 1) return {};

    // reduce all data to consistent length
    data.forEach(d => {
      d.data.forEach(({ x }) => {
        data
          .filter(d2 => d2.id !== d.id)
          .forEach(d2 => {
            if (d2.data.findIndex(({ x: x2 }) => x2 === x) < 0)
              d2.data.push({ x, y: null });
          });
      });
    });

    return { data };
  }),
  withPropsOnChange(["data", "scale"], ({ data: d, scale }) => {
    if (scale === 2)
      return {
        data: d.map(({ data, ...rest }) => {
          let mittelwert = 0;
          data.forEach(({ y }) => {
            mittelwert += y;
          });
          mittelwert /= data.length;

          let standardabweichung = 0;
          data.forEach(({ y }) => {
            standardabweichung += (y - mittelwert) ** 2;
          });
          standardabweichung /= data.length;
          standardabweichung = Math.sqrt(standardabweichung);

          return {
            ...rest,
            standardabweichung,
            mittelwert,
            data: data.map(({ x, y }) => ({
              x,
              y: standardabweichung
                ? Math.round(((y - mittelwert) / standardabweichung) * 1000) /
                  1000
                : 0
            }))
          };
        })
      };

    return { data: d };
  }),
  withPropsOnChange(["data", "average"], ({ data: d, average: a }) => {
    if (a) {
      const filteredData = d.filter(
        ({ id }) => !id.includes(" Min") && !id.includes(" Max")
      );
      const average = {
        id: "Average",
        color: get(
          colorsMaterial,
          [a === 1 ? filteredData.length : 0, "palette", 6],
          "black"
        ),
        data: []
      };

      filteredData.forEach(({ data }) => {
        data.forEach(({ x, y }, i) => {
          if (!average.data[i]) average.data[i] = { x, y };
          else average.data[i].y += y;
        });
      });
      average.data = average.data.map(({ x, y }) => ({
        x,
        y: Math.round((y / filteredData.length) * 1000) / 1000
      }));

      return { data: a === 1 ? [...d, average] : [average] };
    }

    return {};
  }),
  withPropsOnChange(["data", "interpolate"], ({ data, interpolate }) => {
    const tickValues = [];
    let min = Infinity;
    let max = 0;

    if (interpolate) {
      const newData = data.map(d => {
        const data2 = [];

        d.data.forEach(({ x, y }, i) => {
          min = x < min ? x : min;
          max = x > max ? x : max;

          if (i) {
            let x0 = d.data[i - 1].x;
            const y0 = d.data[i - 1].y;
            const m = (y - y0) / (x - x0);

            while (Math.round((x - x0) * 1000) / 1000 > 0.01) {
              x0 += 0.01;

              data2.push({
                x: Math.round(x0 * 1000) / 1000,
                y: Math.round((y - m * (x - x0)) * 1000) / 1000
              });
            }
          }

          data2.push({ x, y });
        });

        return {
          ...d,
          data: data2
        };
      });

      for (let i = min; i < max; i += (max - min) / 20)
        tickValues.push(Math.round(i * 20) / 20);
      tickValues.push(Math.round(max * 20) / 20);

      return {
        data: newData,
        tickValues
      };
    }

    return { data, tickValues };
  }),
  withPropsOnChange(["data", "average"], ({ data: d, average }) => {
    const markers = [];

    d.filter(({ id }) => !id.includes(" Min") && !id.includes(" Max")).forEach(
      ({ data, id }, i) => {
        let min = Infinity;
        let minX;
        let max = -Infinity;
        let maxX;

        if (id === "Average" || !average)
          data.forEach(({ x, y }) => {
            if (y < min) {
              min = y;
              minX = x;
            }
            if (y > max) {
              max = y;
              maxX = x;
            }
          });

        if (minX !== undefined) {
          const index = markers.findIndex(m => m.value === minX);

          if (index >= 0) {
            markers[index].legend = "multiple extrema";
            markers[index].lineStyle.stroke = "black";
          } else
            markers.push({
              axis: "x",
              value: minX,
              lineStyle: {
                stroke: get(colorsMaterial, [i, "palette", 3], "black"),
                strokeWidth: 1
              },
              legend: `min ${id} [${minX}, ${min}]`,
              legendOrientation: "vertical"
            });
        }
        if (maxX !== undefined) {
          const index = markers.findIndex(m => m.value === maxX);

          if (index >= 0) {
            markers[index].legend = "multiple extrema";
            markers[index].lineStyle.stroke = "black";
          } else
            markers.push({
              axis: "x",
              value: maxX,
              lineStyle: {
                stroke: get(colorsMaterial, [i, "palette", 3], "black"),
                strokeWidth: 1
              },
              legend: `max ${id} [${maxX}, ${max}]`,
              legendOrientation: "vertical"
            });
        }
      }
    );

    return {
      markers
    };
  }),
  withPropsOnChange(["data", "tickValues"], ({ data, tickValues }) => {
    let newTicks = [];

    if (!tickValues || !tickValues.length) {
      get(data, [0, "data"], []).forEach(({ x }) => newTicks.push(x));
    } else newTicks = [...tickValues];

    if (newTicks.join("").length > 20)
      newTicks = newTicks.filter((x, i) => i % 2 === 0);

    const numeric = !isNaN(get(newTicks, 0));

    return { tickValues: newTicks, numeric };
  }),
  withHandlers({
    tooltipFormat: ({ scale, data }) => (x, i, p) => {
      const { standardabweichung, mittelwert } =
        data.find(({ id }) => id === p.id) || {};

      switch (scale) {
        case -1:
          return `${x}%`;

        case 1:
          return `${x}%`;

        case 2:
          return standardabweichung
            ? `${x} [${Math.round(
                (x * standardabweichung + mittelwert) * 1000
              ) / 1000}]`
            : x;

        default:
          return x;
      }
    }
  })
);

const Chart = ({
  data,
  tickValues,
  markers,
  large,
  minY,
  maxY,
  scale,
  tooltipFormat,
  selectedFields,
  groupBy,
  average,
  extrema,
  numeric
}) => (
  <Div>
    {!!data.length && (
      <ResponsiveLine
        data={data}
        margin={{
          top: 15,
          right: 30,
          bottom: 85,
          left: 90
        }}
        curve={numeric ? "linear" : "step"}
        markers={extrema ? markers : undefined}
        enableDots={false}
        animate
        colorBy={e => e.color}
        axisBottom={{
          legend: groupBy || "# of cycle",
          legendOffset: 80,
          legendPosition: "center",
          tickValues
        }}
        axisLeft={{
          legend: scale
            ? labelScale[Math.abs(scale)]
            : [average ? "Average" : null, ...selectedFields]
                .filter(x => x)
                .join(", "),
          legendOffset: -65,
          legendPosition: "center"
        }}
        tooltipFormat={tooltipFormat}
        minY={parseInt(minY, 10) || (scale === 2 ? "auto" : 0)}
        maxY={parseInt(maxY, 10) || "auto"}
        legends={[
          {
            anchor: "bottom-left",
            direction: "row",
            translateX: -65,
            translateY: 80,
            itemWidth: !large ? 100 : 180,
            itemHeight: 20
          }
        ]}
      />
    )}
  </Div>
);

export default enhance(Chart);
