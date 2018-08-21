Object.defineProperty(exports, '__esModule', { value: true });

function _interopDefault (ex) { return (ex && (typeof ex === 'object') && 'default' in ex) ? ex['default'] : ex; }

var range = _interopDefault(require('lodash/range'));
var min = _interopDefault(require('lodash/min'));
var max = _interopDefault(require('lodash/max'));
var sumBy = _interopDefault(require('lodash/sumBy'));
var uniq = _interopDefault(require('lodash/uniq'));
var d3Scale = require('d3-scale');
var React = _interopDefault(require('react'));
var PropTypes = _interopDefault(require('prop-types'));
var pure = _interopDefault(require('recompose/pure'));
var core = require('@nivo/core');
var isFunction = _interopDefault(require('lodash/isFunction'));
var d3Format = require('d3-format');
var compose = _interopDefault(require('recompose/compose'));
var withState = _interopDefault(require('recompose/withState'));
var withHandlers = _interopDefault(require('recompose/withHandlers'));
var withPropsOnChange = _interopDefault(require('recompose/withPropsOnChange'));
var reactMotion = require('react-motion');
var legends = require('@nivo/legends');
var sortBy = _interopDefault(require('lodash/sortBy'));
var d3Shape = require('d3-shape');
var defaultProps = _interopDefault(require('recompose/defaultProps'));

/**
 * Generates X scale.
 *
 * @param {Array.<Object>} data
 * @param {number}         width
 * @returns {Function}
 */
var getXScale = function getXScale(data, width) {
    var xLengths = uniq(data.map(function (_ref) {
        var data = _ref.data;
        return data.length;
    }));
    if (xLengths.length > 1) {
        throw new Error(['Found inconsitent data for x,', 'expecting all series to have same length', 'but found: ' + xLengths.join(', ')].join(' '));
    }

    return d3Scale.scalePoint().range([0, width]).domain(data[0].data.map(function (_ref2) {
        var x = _ref2.x;
        return x;
    }));
};

/**
 * Generates Y scale for line chart.
 *
 * @param {Array.<Object>} data
 * @param {number}         height
 * @param {number|string}  minValue
 * @param {number|string}  maxValue
 * @returns {Function}
 */
var getYScale = function getYScale(data, height, minValue, maxValue) {
    var minY = minValue;
    if (minValue === 'auto') {
        minY = min(data.map(function (serie) {
            return min(serie.data.map(function (d) {
                return d.y;
            }));
        }));
    }

    var maxY = maxValue;
    if (maxValue === 'auto') {
        maxY = max(data.map(function (serie) {
            return max(serie.data.map(function (d) {
                return d.y;
            }));
        }));
    }

    return d3Scale.scaleLinear().rangeRound([height, 0]).domain([minY, maxY]);
};

/**
 * Generates Y scale for stacked line chart.
 *
 * @param {Array.<Object>} data
 * @param {Object}         xScale
 * @param {number}         height
 * @param {number|string}  minValue
 * @param {number|string}  maxValue
 * @returns {Function}
 */
var getStackedYScale = function getStackedYScale(data, xScale, height, minValue, maxValue) {
    var minY = minValue;
    if (minValue === 'auto') {
        minY = min(data.map(function (serie) {
            return min(serie.data.map(function (d) {
                return d.y;
            }));
        }));
    }

    var maxY = maxValue;
    if (maxValue === 'auto') {
        maxY = max(range(xScale.domain().length).map(function (i) {
            return sumBy(data, function (serie) {
                return serie.data[i].y;
            });
        }));
    }

    return d3Scale.scaleLinear().rangeRound([height, 0]).domain([minY, maxY]);
};

/**
 * Generates stacked x/y scales.
 *
 * @param {Array}         data
 * @param {number}        width
 * @param {number}        height
 * @param {number|string} minY
 * @param {number|string} maxY
 * @return {{ xScale: Function, yScale: Function }}
 */
var getStackedScales = function getStackedScales(_ref3) {
    var data = _ref3.data,
        width = _ref3.width,
        height = _ref3.height,
        minY = _ref3.minY,
        maxY = _ref3.maxY;

    var xScale = getXScale(data, width);
    var yScale = getStackedYScale(data, xScale, height, minY, maxY);

    return { xScale: xScale, yScale: yScale };
};

/**
 * Generates non stacked x/ scales
 *
 * @param {Array}         data
 * @param {number}        width
 * @param {number}        height
 * @param {number|string} minY
 * @param {number|string} maxY
 * @return {{ xScale: Function, yScale: Function }}
 */
var getScales = function getScales(_ref4) {
    var data = _ref4.data,
        width = _ref4.width,
        height = _ref4.height,
        minY = _ref4.minY,
        maxY = _ref4.maxY;

    var xScale = getXScale(data, width);
    var yScale = getYScale(data, height, minY, maxY);

    return { xScale: xScale, yScale: yScale };
};

/**
 * Generates x/y scales & lines for line chart.
 *
 * @param {Array.<Object>} data
 * @param {Function}       xScale
 * @param {Function}       yScale
 * @param {Function}       color
 * @return {{ xScale: Function, yScale: Function, lines: Array.<Object> }}
 */
var generateLines = function generateLines(data, xScale, yScale, color) {
    return data.map(function (serie) {
        var id = serie.id,
            serieData = serie.data;


        return {
            id: id,
            color: color(serie),
            data: serie,
            points: serieData.map(function (d) {
                return Object.assign({}, d, {
                    value: d.y,
                    x: xScale(d.x),
                    y: yScale(d.y)
                });
            })
        };
    });
};

/**
 * Generates x/y scales & lines for stacked line chart.
 *
 * @param {Array.<Object>} data
 * @param {Function}       xScale
 * @param {Function}       yScale
 * @param {Function}       color
 * @return {{ xScale: Function, yScale: Function, lines: Array.<Object> }}
 */
var generateStackedLines = function generateStackedLines(data, xScale, yScale, color) {
    return data.reduce(function (acc, serie, serieIndex) {
        var previousPoints = serieIndex === 0 ? null : acc[serieIndex - 1].points;

        var id = serie.id,
            serieData = serie.data;


        return [].concat(acc, [{
            id: id,
            color: color(serie),
            data: serie,
            points: serieData.map(function (d, i) {
                if (!previousPoints) {
                    return Object.assign({}, d, {
                        value: d.y,
                        x: d.x,
                        y: d.y
                    });
                }

                return Object.assign({}, d, {
                    value: d.y,
                    x: d.x,
                    y: d.y + previousPoints[i].accY
                });
            }).map(function (d) {
                return {
                    key: d.x,
                    value: d.value,
                    accY: d.y,
                    x: xScale(d.x),
                    y: yScale(d.y)
                };
            })
        }]);
    }, []);
};

var _extends = Object.assign || function (target) {
  for (var i = 1; i < arguments.length; i++) {
    var source = arguments[i];

    for (var key in source) {
      if (Object.prototype.hasOwnProperty.call(source, key)) {
        target[key] = source[key];
      }
    }
  }

  return target;
};

var LineAreas = function LineAreas(_ref) {
    var areaGenerator = _ref.areaGenerator,
        areaOpacity = _ref.areaOpacity,
        lines = _ref.lines,
        animate = _ref.animate,
        motionStiffness = _ref.motionStiffness,
        motionDamping = _ref.motionDamping;

    if (animate !== true) {
        return React.createElement(
            'g',
            null,
            lines.slice(0).reverse().map(function (_ref2) {
                var id = _ref2.id,
                    areaColor = _ref2.color,
                    points = _ref2.points;
                return React.createElement('path', {
                    key: id,
                    d: areaGenerator(points),
                    fill: areaColor,
                    fillOpacity: areaOpacity,
                    strokeWidth: 0
                });
            })
        );
    }

    var springConfig = {
        stiffness: motionStiffness,
        damping: motionDamping
    };

    return React.createElement(
        'g',
        null,
        lines.slice(0).reverse().map(function (_ref3) {
            var id = _ref3.id,
                areaColor = _ref3.color,
                points = _ref3.points;
            return React.createElement(
                core.SmartMotion,
                {
                    key: id,
                    style: function style(spring) {
                        return {
                            d: spring(areaGenerator(points), springConfig),
                            fill: spring(areaColor, springConfig)
                        };
                    }
                },
                function (style) {
                    return React.createElement('path', {
                        key: id,
                        d: style.d,
                        fill: areaColor,
                        fillOpacity: areaOpacity,
                        strokeWidth: 0
                    });
                }
            );
        })
    );
};

LineAreas.propTypes = _extends({
    areaOpacity: PropTypes.number.isRequired
}, core.motionPropTypes);

var LineAreas$1 = pure(LineAreas);

var LineLines = function LineLines(_ref) {
    var lines = _ref.lines,
        lineGenerator = _ref.lineGenerator,
        lineWidth = _ref.lineWidth,
        animate = _ref.animate,
        motionStiffness = _ref.motionStiffness,
        motionDamping = _ref.motionDamping;

    if (animate !== true) {
        return React.createElement(
            'g',
            null,
            lines.map(function (_ref2) {
                var id = _ref2.id,
                    lineColor = _ref2.color,
                    points = _ref2.points;
                return React.createElement('path', {
                    key: id,
                    d: lineGenerator(points),
                    fill: 'none',
                    strokeWidth: lineWidth,
                    stroke: lineColor
                });
            })
        );
    }

    var springConfig = {
        stiffness: motionStiffness,
        damping: motionDamping
    };

    return React.createElement(
        'g',
        null,
        lines.map(function (_ref3) {
            var id = _ref3.id,
                lineColor = _ref3.color,
                points = _ref3.points;
            return React.createElement(
                core.SmartMotion,
                {
                    key: id,
                    style: function style(spring) {
                        return {
                            d: spring(lineGenerator(points), springConfig),
                            stroke: spring(lineColor, springConfig)
                        };
                    }
                },
                function (style) {
                    return React.createElement('path', {
                        key: id,
                        d: style.d,
                        fill: 'none',
                        strokeWidth: lineWidth,
                        stroke: style.stroke
                    });
                }
            );
        })
    );
};

LineLines.propTypes = _extends({
    lineWidth: PropTypes.number.isRequired
}, core.motionPropTypes);

var LineLines$1 = pure(LineLines);

var Chip = function Chip(_ref) {
    var color = _ref.color;
    return React.createElement('span', { style: { display: 'block', width: '12px', height: '12px', background: color } });
};

var LineSlicesItem = function LineSlicesItem(_ref2) {
    var slice = _ref2.slice,
        height = _ref2.height,
        showTooltip = _ref2.showTooltip,
        hideTooltip = _ref2.hideTooltip,
        isHover = _ref2.isHover;
    return React.createElement(
        'g',
        { transform: 'translate(' + slice.x + ', 0)' },
        isHover && React.createElement('line', {
            x1: 0,
            x2: 0,
            y1: 0,
            y2: height,
            stroke: '#000',
            strokeOpacity: 0.35,
            strokeWidth: 1
        }),
        React.createElement('rect', {
            x: -20,
            width: 40,
            height: height,
            fill: '#000',
            fillOpacity: 0,
            onMouseEnter: showTooltip,
            onMouseMove: showTooltip,
            onMouseLeave: hideTooltip
        })
    );
};

LineSlicesItem.propTypes = {
    slice: PropTypes.object.isRequired,
    height: PropTypes.number.isRequired,
    showTooltip: PropTypes.func.isRequired,
    hideTooltip: PropTypes.func.isRequired,
    isHover: PropTypes.bool.isRequired,
    theme: PropTypes.object.isRequired
};

var enhance = compose(withState('isHover', 'setIsHover', false), withPropsOnChange(['slice', 'theme', 'tooltipFormat'], function (_ref3) {
    var slice = _ref3.slice,
        theme = _ref3.theme,
        tooltipFormat = _ref3.tooltipFormat;

    var format = !tooltipFormat || isFunction(tooltipFormat) ? tooltipFormat : d3Format.format(tooltipFormat);
    var hasValues = slice.points.some(function (p) {
        return p.value !== null;
    });

    return {
        tooltip: hasValues ? React.createElement(core.TableTooltip, {
            theme: theme,
            rows: slice.points.filter(function (p) {
                return p.value !== null;
            }).map(function (p, i) {
                return [React.createElement(Chip, { color: p.color }), p.id, format ? format(p.value, i, {...p}) : p.value];
            })
        }) : null
    };
}), withHandlers({
    showTooltip: function showTooltip(_ref4) {
        var _showTooltip = _ref4.showTooltip,
            setIsHover = _ref4.setIsHover,
            tooltip = _ref4.tooltip;
        return function (e) {
            setIsHover(true);
            _showTooltip(tooltip, e);
        };
    },
    hideTooltip: function hideTooltip(_ref5) {
        var _hideTooltip = _ref5.hideTooltip,
            setIsHover = _ref5.setIsHover;
        return function () {
            setIsHover(false);
            _hideTooltip();
        };
    }
}), pure);

var LineSlicesItem$1 = enhance(LineSlicesItem);

var LineSlices = function LineSlices(_ref) {
    var slices = _ref.slices,
        height = _ref.height,
        showTooltip = _ref.showTooltip,
        hideTooltip = _ref.hideTooltip,
        theme = _ref.theme,
        tooltipFormat = _ref.tooltipFormat;
    return React.createElement(
        'g',
        null,
        slices.map(function (slice) {
            return React.createElement(LineSlicesItem$1, {
                key: slice.id,
                slice: slice,
                height: height,
                showTooltip: showTooltip,
                hideTooltip: hideTooltip,
                theme: theme,
                tooltipFormat: tooltipFormat
            });
        })
    );
};

LineSlices.propTypes = {
    slices: PropTypes.arrayOf(PropTypes.shape({
        id: PropTypes.oneOfType([PropTypes.number, PropTypes.string]).isRequired,
        x: PropTypes.number.isRequired,
        points: PropTypes.arrayOf(PropTypes.shape({
            id: PropTypes.oneOfType([PropTypes.number, PropTypes.string]).isRequired,
            value: PropTypes.oneOfType([PropTypes.number, PropTypes.string]),
            color: PropTypes.string.isRequired
        })).isRequired
    })).isRequired,
    height: PropTypes.number.isRequired,
    showTooltip: PropTypes.func.isRequired,
    hideTooltip: PropTypes.func.isRequired,
    theme: PropTypes.object.isRequired
};

var LineSlices$1 = pure(LineSlices);

var LineDots = function LineDots(_ref) {
    var lines = _ref.lines,
        symbol = _ref.symbol,
        size = _ref.size,
        color = _ref.color,
        borderWidth = _ref.borderWidth,
        borderColor = _ref.borderColor,
        enableLabel = _ref.enableLabel,
        label = _ref.label,
        labelFormat = _ref.labelFormat,
        labelYOffset = _ref.labelYOffset,
        theme = _ref.theme,
        animate = _ref.animate,
        motionStiffness = _ref.motionStiffness,
        motionDamping = _ref.motionDamping;

    var getLabel = core.getLabelGenerator(label, labelFormat);

    var points = lines.reduce(function (acc, line) {
        var id = line.id,
            points = line.points;


        return [].concat(acc, points.filter(function (point) {
            return point.value !== null;
        }).map(function (point) {
            var pointData = {
                serie: { id: id },
                x: point.key,
                y: point.value
            };

            return {
                key: id + '.' + point.x,
                x: point.x,
                y: point.y,
                fill: color(line),
                stroke: borderColor(line),
                label: enableLabel ? getLabel(pointData) : null
            };
        }));
    }, []);

    if (animate !== true) {
        return React.createElement(
            'g',
            null,
            points.map(function (point) {
                return React.createElement(core.DotsItem, {
                    key: point.key,
                    x: point.x,
                    y: point.y,
                    symbol: symbol,
                    size: size,
                    color: point.fill,
                    borderWidth: borderWidth,
                    borderColor: point.stroke,
                    label: point.label,
                    labelYOffset: labelYOffset,
                    theme: theme
                });
            })
        );
    }
    var springConfig = {
        motionDamping: motionDamping,
        motionStiffness: motionStiffness
    };

    return React.createElement(
        reactMotion.TransitionMotion,
        {
            styles: points.map(function (point) {
                return {
                    key: point.key,
                    data: point,
                    style: {
                        x: reactMotion.spring(point.x, springConfig),
                        y: reactMotion.spring(point.y, springConfig),
                        size: reactMotion.spring(size, springConfig)
                    }
                };
            })
        },
        function (interpolatedStyles) {
            return React.createElement(
                'g',
                null,
                interpolatedStyles.map(function (_ref2) {
                    var key = _ref2.key,
                        style = _ref2.style,
                        point = _ref2.data;
                    return React.createElement(core.DotsItem, _extends({
                        key: key
                    }, style, {
                        symbol: symbol,
                        color: point.fill,
                        borderWidth: borderWidth,
                        borderColor: point.stroke,
                        label: point.label,
                        labelYOffset: labelYOffset,
                        theme: theme
                    }));
                })
            );
        }
    );
};

LineDots.propTypes = _extends({
    lines: PropTypes.arrayOf(PropTypes.shape({
        id: PropTypes.string.isRequired
    })),

    symbol: PropTypes.func,
    size: PropTypes.number.isRequired,
    color: PropTypes.func.isRequired,
    borderWidth: PropTypes.number.isRequired,
    borderColor: PropTypes.func.isRequired,

    // labels
    enableLabel: PropTypes.bool.isRequired,
    label: PropTypes.oneOfType([PropTypes.string, PropTypes.func]).isRequired,
    labelFormat: PropTypes.string,
    labelYOffset: PropTypes.number,

    // theming
    theme: PropTypes.shape({
        dots: PropTypes.shape({
            textColor: PropTypes.string.isRequired,
            fontSize: PropTypes.string.isRequired
        }).isRequired
    }).isRequired

}, core.motionPropTypes);

LineDots.defaultProps = {
    // labels
    enableLabel: false,
    label: 'y'
};

var LinePropTypes = {
    // data
    data: PropTypes.arrayOf(PropTypes.shape({
        id: PropTypes.string.isRequired,
        data: PropTypes.arrayOf(PropTypes.shape({
            x: PropTypes.oneOfType([PropTypes.number, PropTypes.string]).isRequired,
            y: PropTypes.oneOfType([PropTypes.number, PropTypes.string])
        })).isRequired
    })).isRequired,

    stacked: PropTypes.bool.isRequired,
    curve: core.lineCurvePropType.isRequired,
    areaGenerator: PropTypes.func.isRequired,
    lineGenerator: PropTypes.func.isRequired,

    lines: PropTypes.array.isRequired,
    slices: PropTypes.array.isRequired,

    minY: PropTypes.oneOfType([PropTypes.number, PropTypes.string, PropTypes.oneOf(['auto'])]).isRequired,
    maxY: PropTypes.oneOfType([PropTypes.number, PropTypes.string, PropTypes.oneOf(['auto'])]).isRequired,
    xScale: PropTypes.func.isRequired, // computed
    yScale: PropTypes.func.isRequired, // computed

    // axes & grid
    axisTop: PropTypes.object,
    axisRight: PropTypes.object,
    axisBottom: PropTypes.object,
    axisLeft: PropTypes.object,
    enableGridX: PropTypes.bool.isRequired,
    enableGridY: PropTypes.bool.isRequired,

    // dots
    enableDots: PropTypes.bool.isRequired,
    dotSymbol: PropTypes.func,
    dotSize: PropTypes.number.isRequired,
    dotColor: PropTypes.any.isRequired,
    dotBorderWidth: PropTypes.number.isRequired,
    dotBorderColor: PropTypes.any.isRequired,
    enableDotLabel: PropTypes.bool.isRequired,

    // markers
    markers: PropTypes.arrayOf(PropTypes.shape({
        axis: PropTypes.oneOf(['x', 'y']).isRequired,
        value: PropTypes.oneOfType([PropTypes.number, PropTypes.string]).isRequired,
        style: PropTypes.object
    })),

    // styling
    getColor: PropTypes.func.isRequired,
    enableArea: PropTypes.bool.isRequired,
    areaOpacity: PropTypes.number.isRequired,
    lineWidth: PropTypes.number.isRequired,
    defs: PropTypes.arrayOf(PropTypes.shape({
        id: PropTypes.string.isRequired
    })).isRequired,

    // interactivity
    isInteractive: PropTypes.bool.isRequired,
    enableStackTooltip: PropTypes.bool.isRequired,
    tooltipFormat: PropTypes.oneOfType([PropTypes.func, PropTypes.string]),

    legends: PropTypes.arrayOf(PropTypes.shape(legends.LegendPropShape)).isRequired
};

var LineDefaultProps = {
    indexBy: 'id',
    keys: ['value'],

    stacked: false,
    curve: 'linear',

    // scales
    minY: 0,
    maxY: 'auto',

    // axes & grid
    axisBottom: {},
    axisLeft: {},
    enableGridX: true,
    enableGridY: true,

    // dots
    enableDots: true,
    dotSize: 6,
    dotColor: 'inherit',
    dotBorderWidth: 0,
    dotBorderColor: 'inherit',
    enableDotLabel: false,

    // styling
    colors: 'nivo',
    colorBy: 'id',
    enableArea: false,
    areaOpacity: 0.2,
    lineWidth: 2,
    defs: [],

    // interactivity
    isInteractive: true,
    enableStackTooltip: true,

    legends: []
};

var Line = function Line(_ref) {
    var lines = _ref.lines,
        lineGenerator = _ref.lineGenerator,
        areaGenerator = _ref.areaGenerator,
        xScale = _ref.xScale,
        yScale = _ref.yScale,
        slices = _ref.slices,
        margin = _ref.margin,
        width = _ref.width,
        height = _ref.height,
        outerWidth = _ref.outerWidth,
        outerHeight = _ref.outerHeight,
        axisTop = _ref.axisTop,
        axisRight = _ref.axisRight,
        axisBottom = _ref.axisBottom,
        axisLeft = _ref.axisLeft,
        enableGridX = _ref.enableGridX,
        enableGridY = _ref.enableGridY,
        lineWidth = _ref.lineWidth,
        enableArea = _ref.enableArea,
        areaOpacity = _ref.areaOpacity,
        enableDots = _ref.enableDots,
        dotSymbol = _ref.dotSymbol,
        dotSize = _ref.dotSize,
        dotColor = _ref.dotColor,
        dotBorderWidth = _ref.dotBorderWidth,
        dotBorderColor = _ref.dotBorderColor,
        enableDotLabel = _ref.enableDotLabel,
        dotLabel = _ref.dotLabel,
        dotLabelFormat = _ref.dotLabelFormat,
        dotLabelYOffset = _ref.dotLabelYOffset,
        markers = _ref.markers,
        theme = _ref.theme,
        animate = _ref.animate,
        motionStiffness = _ref.motionStiffness,
        motionDamping = _ref.motionDamping,
        isInteractive = _ref.isInteractive,
        tooltipFormat = _ref.tooltipFormat,
        enableStackTooltip = _ref.enableStackTooltip,
        legends$$1 = _ref.legends;

    var motionProps = {
        animate: animate,
        motionDamping: motionDamping,
        motionStiffness: motionStiffness
    };

    return React.createElement(
        core.Container,
        { isInteractive: isInteractive, theme: theme },
        function (_ref2) {
            var showTooltip = _ref2.showTooltip,
                hideTooltip = _ref2.hideTooltip;
            return React.createElement(
                core.SvgWrapper,
                { width: outerWidth, height: outerHeight, margin: margin },
                React.createElement(core.Grid, _extends({
                    theme: theme,
                    width: width,
                    height: height,
                    xScale: enableGridX ? xScale : null,
                    yScale: enableGridY ? yScale : null
                }, motionProps)),
                React.createElement(core.CartesianMarkers, {
                    markers: markers,
                    width: width,
                    height: height,
                    xScale: xScale,
                    yScale: yScale,
                    theme: theme
                }),
                React.createElement(core.Axes, _extends({
                    xScale: xScale,
                    yScale: yScale,
                    width: width,
                    height: height,
                    theme: theme,
                    top: axisTop,
                    right: axisRight,
                    bottom: axisBottom,
                    left: axisLeft
                }, motionProps)),
                enableArea && React.createElement(LineAreas$1, _extends({
                    areaGenerator: areaGenerator,
                    areaOpacity: areaOpacity,
                    lines: lines
                }, motionProps)),
                React.createElement(LineLines$1, _extends({
                    lines: lines,
                    lineGenerator: lineGenerator,
                    lineWidth: lineWidth
                }, motionProps)),
                isInteractive && enableStackTooltip && React.createElement(LineSlices$1, {
                    slices: slices,
                    height: height,
                    showTooltip: showTooltip,
                    hideTooltip: hideTooltip,
                    theme: theme,
                    tooltipFormat: tooltipFormat
                }),
                enableDots && React.createElement(LineDots, _extends({
                    lines: lines,
                    symbol: dotSymbol,
                    size: dotSize,
                    color: core.getInheritedColorGenerator(dotColor),
                    borderWidth: dotBorderWidth,
                    borderColor: core.getInheritedColorGenerator(dotBorderColor),
                    enableLabel: enableDotLabel,
                    label: dotLabel,
                    labelFormat: dotLabelFormat,
                    labelYOffset: dotLabelYOffset,
                    theme: theme
                }, motionProps)),
                legends$$1.map(function (legend, i) {
                    var legendData = lines.map(function (line) {
                        return {
                            label: line.id,
                            fill: line.color
                        };
                    }).reverse();

                    return React.createElement(legends.BoxLegendSvg, _extends({
                        key: i
                    }, legend, {
                        containerWidth: width,
                        containerHeight: height,
                        data: legendData
                    }));
                })
            );
        }
    );
};

Line.propTypes = LinePropTypes;

var enhance$1 = compose(defaultProps(LineDefaultProps), core.withTheme(), core.withColors(), core.withDimensions(), core.withMotion(), withPropsOnChange(['curve', 'height'], function (_ref3) {
    var curve = _ref3.curve,
        height = _ref3.height;
    return {
        areaGenerator: d3Shape.area().x(function (d) {
            return d.x;
        }).y0(height).y1(function (d) {
            return d.y;
        }).curve(core.curveFromProp(curve)),
        lineGenerator: d3Shape.line().defined(function (d) {
            return d.value !== null;
        }).x(function (d) {
            return d.x;
        }).y(function (d) {
            return d.y;
        }).curve(core.curveFromProp(curve))
    };
}), withPropsOnChange(['data', 'stacked', 'width', 'height', 'minY', 'maxY'], function (_ref4) {
    var data = _ref4.data,
        stacked = _ref4.stacked,
        width = _ref4.width,
        height = _ref4.height,
        margin = _ref4.margin,
        minY = _ref4.minY,
        maxY = _ref4.maxY;

    var scales = void 0;
    var args = { data: data, width: width, height: height, minY: minY, maxY: maxY };
    if (stacked === true) {
        scales = getStackedScales(args);
    } else {
        scales = getScales(args);
    }

    return _extends({
        margin: margin,
        width: width,
        height: height
    }, scales);
}), withPropsOnChange(['getColor', 'xScale', 'yScale'], function (_ref5) {
    var data = _ref5.data,
        stacked = _ref5.stacked,
        xScale = _ref5.xScale,
        yScale = _ref5.yScale,
        getColor = _ref5.getColor;

    var lines = void 0;
    if (stacked === true) {
        lines = generateStackedLines(data, xScale, yScale, getColor);
    } else {
        lines = generateLines(data, xScale, yScale, getColor);
    }

    var slices = xScale.domain().map(function (id, i) {
        var points = sortBy(lines.map(function (line) {
            return {
                id: line.id,
                value: line.points[i].value,
                y: line.points[i].y,
                color: line.color
            };
        }), 'y');

        return {
            id: id,
            x: xScale(id),
            points: points
        };
    });

    return { lines: lines, slices: slices };
}), pure);

var enhancedLine = enhance$1(Line);
enhancedLine.displayName = 'enhance(Line)';

var ResponsiveLine = (function (props) {
    return React.createElement(
        core.ResponsiveWrapper,
        null,
        function (_ref) {
            var width = _ref.width,
                height = _ref.height;
            return React.createElement(enhancedLine, _extends({ width: width, height: height }, props));
        }
    );
});

exports.Line = enhancedLine;
exports.ResponsiveLine = ResponsiveLine;
exports.LinePropTypes = LinePropTypes;
exports.LineDefaultProps = LineDefaultProps;
