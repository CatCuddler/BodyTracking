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
  withPropsOnChange(['files', 'folder'], ({ files, folder }) => ({
    folders: files
      .map(file => file.folder)
      .filter((x, index, self) => self.indexOf(x) === index),
    files: files.filter(file => file.folder === folder)
  })),
  withState('selectedFiles', 'setSelectedFiles', ({ files }) => [
    get(files, [0, 'name'])
  ]),
  withPropsOnChange(['files'], ({ files }) => ({
    fields:
      intersect(...files.map(file => Object.keys(get(file, 'values', {})))) ||
      []
  })),
  withState('selectedFields', 'setSelectedFields', ({ fields }) => [
    get(fields, 0)
  ]),
  withHandlers({
    onClickField: ({ selectedFiles, selectedFields, setSelectedFields }) => (
      e,
      field
    ) =>
      e.shiftKey && selectedFiles.length === 1
        ? setSelectedFields(
            selectedFields.includes(field)
              ? selectedFields.filter(x => x !== field)
              : [...selectedFields, field]
          )
        : setSelectedFields([field]),
    onClickFile: ({
      selectedFields,
      setSelectedFields,
      selectedFiles,
      setSelectedFiles
    }) => (e, file) => {
      if (
        e.shiftKey &&
        (selectedFiles.length > 1 || !selectedFiles.includes(file))
      ) {
        setSelectedFiles(
          selectedFiles.includes(file)
            ? selectedFiles.filter(x => x !== file)
            : [...selectedFiles, file]
        );
        setSelectedFields([get(selectedFields, 0)]);
      } else setSelectedFiles([file]);
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
  fields,
  selectedFields,
  onClickField,
  groupBy,
  setGroupBy
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
      folders={folders}
      folder={folder}
      setFolder={setFolder}
      files={files.filter(file => selectedFiles.includes(file.name))}
      fields={fields}
      selectedFields={selectedFields}
      onClickField={onClickField}
      groupBy={groupBy}
      setGroupBy={setGroupBy}
    />

    <div style={{ flexGrow: 1, display: 'flex' }}>
      <Files
        files={files}
        selectedFiles={selectedFiles}
        onClick={onClickFile}
        groupBy={groupBy}
      />
      <Chart
        files={files.filter(file => selectedFiles.includes(file.name))}
        fields={selectedFields}
        groupBy={groupBy}
      />
    </div>
  </div>
);

export default enhance(App);
