import React from 'react';
import { compose, withState, withPropsOnChange, withHandlers } from 'recompose';
import { get } from 'lodash';
import Nav from './nav';
import Files from './files';
import Chart from './chart';

const intersect = (xs, ys, ...rest) =>
  ys === undefined
    ? xs
    : intersect(xs.filter(x => ys.some(y => y === x), ...rest));

const enhance = compose(
  withState('groupBy', 'setGroupBy', 'mode'),
  withState('folder', 'setFolder', 'json'),
  withState('acc', 'setAcc'),
  withState('min', 'setMin'),
  withState('max', 'setMax'),
  withState('scale', 'setScale'),
  withPropsOnChange(['files', 'folder'], ({ files, folder }) => ({
    folders: files
      .map(file => file.folder)
      .filter((x, index, self) => self.indexOf(x) === index),
    files: files.filter(file => file.folder === folder)
  })),
  withState('selectedFiles', 'setSelectedFiles', ({ files }) => [
    get(files, [0, 'name'])
  ]),
  withPropsOnChange(['selectedFiles', 'files'], ({ selectedFiles, files }) => ({
    fields:
      intersect(
        ...selectedFiles.map(fileName => {
          const file = files.find(x => x.name === fileName);

          return Object.keys(get(file, 'values', {}));
        })
      ) || []
  })),
  withState('selectedFields', 'setSelectedFields', ({ fields }) => [
    get(fields, 0)
  ]),
  withPropsOnChange(['selectedFields'], ({ selectedFields, setScale }) => {
    setScale(selectedFields.length > 1);
  }),
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
  groupBy,
  setGroupBy,
  acc,
  setAcc,
  min,
  setMin,
  max,
  setMax,
  scale,
  setScale
}) => (
  <div
    style={{
      display: 'flex',
      flexDirection: 'column',
      height: '100vh',
      padding: '2rem'
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
      groupBy={groupBy}
      setGroupBy={setGroupBy}
      acc={acc}
      setAcc={setAcc}
      min={min}
      setMin={setMin}
      max={max}
      setMax={setMax}
      scale={scale}
      setScale={setScale}
    />

    <div style={{ flexGrow: 1, display: 'flex' }}>
      <Files
        acc={acc}
        files={files}
        selectedFiles={selectedFiles}
        onClickFile={onClickFile}
        onClickFiles={onClickFiles}
        groupBy={groupBy}
      />
      <Chart
        files={files}
        selectedFiles={selectedFiles}
        selectedFields={selectedFields}
        groupBy={groupBy}
        acc={acc}
        min={min}
        max={max}
        scale={scale}
      />
    </div>
  </div>
);

export default enhance(App);
