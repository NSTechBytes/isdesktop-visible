# isdesktop-visible

A Node.js wrapper around a native Windows binary that detects when the desktop is shown or hidden (e.g., via Show Desktop or Win+D).

## Installation

```bash
npm install isdesktop-visible
````

## Usage

```js
const { monitorDesktop } = require('isdesktop-visible');

const proc = monitorDesktop((state, line) => {
    console.log(`Desktop is now ${state}`);
    console.log(`Raw output: ${line}`);
});
```

## Notes

* Only works on **Windows**.
* Requires `isdesktop-visible.exe` in the `bin/Release` folder.
* You can stop the process manually using `proc.kill()`.



### 4. ✅ Usage in Another Project


```js
const { monitorDesktop } = require('isdesktop-visible');

monitorDesktop((state, raw) => {
    if (state === 'visible') {
        console.log('User has minimized all windows!');
    } else {
        console.log('User has focused on a window again.');
    }
});

