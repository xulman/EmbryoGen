# About
This is essentially a [Scenery](https://github.com/scenerygraphics/scenery) 'client'
that [ZeroMQ](https://github.com/zeromq/jeromq)-ish listens for and displays current
geometry of the simulated agents. The communication is based on own/proprietary
(and simple) network protocol.

One can find the simulator that feeds this display in the `../DrosoGen` folder.

# Installing and starting this client
```
git clone https://github.com/xulman/EmbryoGen.git
cd EmbryoGen/SimViewer/
mvn dependency:copy-dependencies package
cd target
java -cp "SimViewer-1.0-SNAPSHOT.jar:dependency/*" de.mpicbg.ulman.simviewer.StartUpScene
```
