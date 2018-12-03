import React from "react";
import { compose, withState, withPropsOnChange, withHandlers } from "recompose";
import { get } from "lodash";
import Nav from "./nav";
import Files from "./files";
import Chart from "./chart";

const intersect = (xs, ys, ...rest) =>
  ys === undefined
    ? xs
    : intersect(xs.filter(x => ys.some(y => y === x), ...rest));

const enhance = compose(
  withState("sortBy", "setSortBy", "mode"),
  withState("groupBy", "setGroupBy"),
  withState("folder", "setFolder", "json"),
  withState("min", "setMin"),
  withState("max", "setMax"),
  withState("scale", "setScale"),
  withState("minY", "setMinY"),
  withState("maxY", "setMaxY"),
  withState("average", "setAverage"),
  withState("extrema", "setExtrema"),
  withState("interpolate", "setInterpolate"),
  withPropsOnChange(["files", "folder"], ({ files, folder }) => ({
    folders: files
      .map(file => file.folder)
      .filter((x, index, self) => self.indexOf(x) === index),
    files: files.filter(file => file.folder === folder)
  })),
  withState("selectedFiles", "setSelectedFiles", ({ files }) => [
    get(files, [0, "name"])
  ]),
  withPropsOnChange(["selectedFiles", "files"], ({ selectedFiles, files }) => ({
    fields:
      intersect(
        ...selectedFiles.map(fileName => {
          const file = files.find(x => x.name === fileName);

          return Object.keys(get(file, "values", {}));
        })
      ) || []
  })),
  withState("selectedFields", "setSelectedFields", ({ fields }) => [
    get(fields, 0) || "Iterations"
  ]),
  withPropsOnChange(
    ["selectedFields"],
    ({ selectedFields, scale, setScale }) => {
      if (!scale && selectedFields.length > 1) setScale(-1);
      else if (scale === -1 && selectedFields.length <= 1) setScale(0);
    }
  ),
  withHandlers({
    onClickField: ({ selectedFields, setSelectedFields }) => (e, field) =>
      e.shiftKey
        ? setSelectedFields(
            selectedFields.includes(field)
              ? selectedFields.filter(x => x !== field)
              : [...selectedFields, field]
          )
        : setSelectedFields([field]),
    onClickFile: ({ selectedFiles, setSelectedFiles }) => (e, file) => {
      if (
        e.shiftKey &&
        (selectedFiles.length > 1 || !selectedFiles.includes(file))
      )
        setSelectedFiles(
          selectedFiles.includes(file)
            ? selectedFiles.filter(x => x !== file)
            : [...selectedFiles, file]
        );
      else setSelectedFiles([file]);
    },
    onClickFiles: ({ selectedFiles, setSelectedFiles }) => (e, files) => {
      if (e.shiftKey)
        setSelectedFiles(
          [...selectedFiles, ...files].filter(
            (file, index, self) => self.indexOf(file) === index
          )
        );
      else setSelectedFiles(files);
    }
  })
);

const App = ({
  folders,
  folder,
  setFolder,
  files,
  selectedFiles,
  onClickFile,
  onClickFiles,
  fields,
  selectedFields,
  onClickField,
  sortBy,
  setSortBy,
  groupBy,
  setGroupBy,
  min,
  setMin,
  max,
  setMax,
  minY,
  setMinY,
  maxY,
  setMaxY,
  scale,
  setScale,
  interpolate,
  setInterpolate,
  average,
  setAverage,
  extrema,
  setExtrema
}) => (
  <div
    style={{
      display: "flex",
      flexDirection: "column",
      height: "100vh",
      padding: "2rem"
    }}
  >
    <Nav
      selectedFiles={selectedFiles}
      fields={fields}
      selectedFields={selectedFields}
      onClickField={onClickField}
      folders={folders}
      folder={folder}
      setFolder={setFolder}
      sortBy={sortBy}
      setSortBy={setSortBy}
      groupBy={groupBy}
      setGroupBy={setGroupBy}
      min={min}
      setMin={setMin}
      max={max}
      setMax={setMax}
      setMinY={setMinY}
      setMaxY={setMaxY}
      scale={scale}
      setScale={setScale}
      interpolate={interpolate}
      setInterpolate={setInterpolate}
      average={average}
      setAverage={setAverage}
      extrema={extrema}
      setExtrema={setExtrema}
    />

    <div style={{ flexGrow: 1, display: "flex" }}>
      <Files
        files={files}
        selectedFiles={selectedFiles}
        onClickFile={onClickFile}
        onClickFiles={onClickFiles}
        sortBy={sortBy}
        groupBy={groupBy}
      />
      <Chart
        files={files}
        selectedFiles={selectedFiles}
        selectedFields={selectedFields}
        sortBy={sortBy}
        groupBy={groupBy}
        min={min}
        max={max}
        minY={minY}
        maxY={maxY}
        scale={scale}
        interpolate={interpolate}
        setInterpolate={setInterpolate}
        average={average}
        extrema={extrema}
      />
    </div>
  </div>
);

export default enhance(App);
