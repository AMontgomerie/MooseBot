MooseBot
========

An AI system for StarCraft: Broodwar using BWAPI

Running the compiled AI (MooseBot.dll):
- install StarCraft: Broodwar
- install BWAPI 3.7.4 (https://github.com/bwapi/bwapi)
- copy MooseBot.dll into StarCraft/bwapi-data/AI
- run Chaoslauncher (comes with BWAPI), tick BWAPI Injector (1.16.1) RELEASE and click Start

Compiling from source code:
- install Visual Studio 2010 and 2012
- install BWAPI 3.7.4 (https://github.com/bwapi/bwapi)
- install boost library (http://www.boost.org/)
- set environment variables to point to point to install directories for BWAPI and Boost
   - BWAPI_DIR - BWAPI directory
   - BOOST_DIR - BOOST directory
- open MooseBot/MooseBot/MooseBot.sln in VS 2012
- make sure platform toolset is set to v90
- set to release mode
- build the project
- follow the instructions above for running the dll file that has been comiled
