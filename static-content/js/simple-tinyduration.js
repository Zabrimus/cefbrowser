
function parseWithTinyDuration(durationStr) {
    let InvalidDurationError = new Error('Invalid duration');

    function parseNum(s) {
        if (s === '' || s === undefined || s === null) {
            return undefined;
        }
        return parseFloat(s.replace(',', '.'));
    }

    const units = [
        { unit: 'years', symbol: 'Y' },
        { unit: 'months', symbol: 'M' },
        { unit: 'weeks', symbol: 'W' },
        { unit: 'days', symbol: 'D' },
        { unit: 'hours', symbol: 'H' },
        { unit: 'minutes', symbol: 'M' },
        { unit: 'seconds', symbol: 'S' },
    ];

    const r = (name, unit) => `((?<${name}>-?\\d*[\\.,]?\\d+)${unit})?`;
    const durationRegex = new RegExp([
        '(?<negative>-)?P',
        r('years', 'Y'),
        r('months', 'M'),
        r('weeks', 'W'),
        r('days', 'D'),
        '(T',
        r('hours', 'H'),
        r('minutes', 'M'),
        r('seconds', 'S'),
        ')?', // end optional time
    ].join(''));

    const match = durationRegex.exec(durationStr);
    if (!match || !match.groups) {
        throw InvalidDurationError;
    }
    let empty = true;
    let decimalFractionCount = 0;
    const values = {};
    for (const { unit } of units) {
        if (match.groups[unit]) {
            empty = false;
            values[unit] = parseNum(match.groups[unit]);
        }
    }
    if (empty) {
        throw InvalidDurationError;
    }
    const duration = values;
    if (match.groups.negative) {
        duration.negative = true;
    }
    return duration;
}

// we need only hours, minutes, seconds
function durationToSeconds(duration) {
    let seconds = 0;
    if (duration.hours !== undefined) {
        seconds += duration.hours * 60 * 60;
    }

    if (duration.minutes !== undefined) {
        seconds += duration.minutes * 60;
    }

    if (duration.seconds !== undefined) {
        seconds += duration.seconds;
    }

    return seconds;
}