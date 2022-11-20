const fs = require('fs');
const path = require('path');
fs.readFile(path.join(__dirname, '../dist/capture.worker.js'), 'utf8', (err, data) => {
    if (err) {
        return console.log(err);
    }
    const result = data.replace(/var[\s]out[\s]=[\s]Module\['print'\][\s]\|\|[\s]console\.log\.bind\(console\)/g,
        'var out = Module["print"] || (() => {})');

    fs.writeFile(path.join(__dirname, '../dist/capture.worker.js'), result, 'utf8', err => {
        if (err) {
            return console.log(err);
        }
    });
});
