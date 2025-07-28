### Running the Test App

I'm assuming you have `npm` available at the command-line, if not, you're not going to make much progress ;)

#### Install the dependencies

Navigate to the `react` project and install the dependencies:
```shell
cd react
npm install
```

#### Sym-link the `ipc.js` file

To avoid having multiple copies of the `ipc.js` script lying around, I link the primary source into this
project's `src/` folder. It's imported in a couple of the React scripts and the build will fail if it can't
find the file.
```shell
ln -s "$(readlink -f ../ipcjs/src/ipc.js)" ./src/ipc.js
```

#### Run the build

```shell
npm run build
```


#### Run the app

The test simply rides on the coat tails of the `q/serve.q` script in the `ipcjs` project, so we have to
adjust the `.h.HOME` value to point to the `build/` directory created in the previous step.
```shell
qq ../ipcjs/q/serve.q
```
```q
q).h.HOME:first system"readlink -f build"
```
Verify the port it's listening on:
```q
q)\p
30097
```

### Load the page

Open [http://localhost:30097/index.html](http://localhost:30097/index.html). I haven't configured a 
default file so you do have to specify the `/index.html` path

