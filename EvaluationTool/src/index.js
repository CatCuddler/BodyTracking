import React from "react";
import ReactDOM from "react-dom";
import "semantic-ui-css/semantic.min.css";
import { get } from "lodash";
import { Provider } from "react-fela";
import { createRenderer } from "fela";
import App from "./app";
import registerServiceWorker from "./registerServiceWorker";
import ikMode from "./ik-modes";

const renderer = createRenderer();

const req = require.context("../json");
const sources = req.keys();

const getConfigIndex = file =>
  sources.findIndex(
    source => source === file.split("evaluationData_").join("evaluationConfig_")
  );

const files = sources
  .filter(
    // filter all evaluationData_x.json-files, which have an config-file
    file =>
      file.includes(".json") &&
      file.includes("evaluationData_") &&
      getConfigIndex(file) >= 0
  )
  .map(file => {
    // get data-source an config-source
    const source = req(file);
    const config = req(get(sources, getConfigIndex(file)));

    // get name & folder
    const path = file.split("/");
    const name = path[path.length - 1];
    path.splice(path.length - 1, 1);
    path[0] = "json";
    const folder = path.join("/");

    // get values from evaluationData_x.json
    const values = {};
    let length = 0;

    source.forEach(value => {
      Object.keys(value)
        .filter(x => x)
        .forEach(field => {
          if (!values[field]) values[field] = [];

          values[field].push(value[field]);
        });

      length += 1;
    });

    return {
      name,
      folder,
      errorPos: parseFloat(config["Error Pos Max"]),
      errorRot: parseFloat(config["Error Rot Max"]),
      steps: parseFloat(config["Steps Max"]),
      lambda: Math.round(config.lambda * 10000) / 10000,
      file: config.File,
      group: config.group,
      orientation: config["with Orientation"] === "1",
      mode: ikMode[config["IK Mode"]] || "NA",
      modeNumber: parseInt(config["IK Mode"], 10),
      values,
      length
    };
  });

ReactDOM.render(
  <Provider renderer={renderer}>
    <App files={files} />
  </Provider>,
  document.getElementById("root")
);
registerServiceWorker();
