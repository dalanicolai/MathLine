#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-avoid-c-arrays"
//
//  MathLine
//
//  Created by Robert Jacobson on 12/14/14.
//  Copyright (c) 2014 Robert Jacobson. All rights reserved.
//

#include <iostream>
#include "popl.hpp"
#include "mlbridge.h"

#define CONTINUE 0
#define QUIT_WITH_SUCCESS 1
#define QUIT_WITH_ERROR 2

/// Copy `std::string` into an owning C-style string.
char *copyDataFromString(const std::string &str){
    char *new_string = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), new_string);
    new_string[str.size()] = '\0';
    return new_string;
}

bool check_and_exit = false;

int ParseProgramOptions(MLBridge &bridge, int argc, const char * argv[]){

    popl::Switch helpOption("h", "help", "Produce help message.");
    popl::Switch checkOption("c", "check", "Establish that the connection to the kernel\nworks, then exit.");
    popl::Value<bool> mainloopOption("m", "mainloop",
                                     "Boolean. Whether or not to use the kernel's\nMain Loop which keeps track of session\nhistory with In[#] and Out[#] variables.\nDefaults to true.", true, &bridge.useMainLoop);
    popl::Value<std::string> promptOption("p", "prompt", "String. The prompt presented to the user for\ninput. When inoutstrings is true, this is\ntypically the empty string.", "", &bridge.prompt);
    popl::Value<bool> iostringsOption("i", "inoutstrings", "Boolean. Whether or not to print the \n\"In[#]:=\" and \"Out[#]=\" strings. When\nmainloop is false this option does nothing.\nDefaults to true.", true, &bridge.showInOutStrings);
    popl::Value<std::string> linknameOption("n", "linkname", "String. The call string to set up the link.\nDefaults to \"math -" MMANAME_LOWER "\".", "math -" MMANAME_LOWER);
    popl::Value<std::string> linkmodeOption("l", "linkmode", "String. The " MMANAME " link mode. The default\nlaunches a new kernel which is almost\ncertainly what you want. It should be\npossible, however, to connect to a pre\nexisting kernel. Defaults to \"linklaunch\".", "linklaunch");
    popl::Value<bool> getlineOption("g", "usegetline", "Boolean. If set to false, we use readline-\nlike input with command history and emacs-\nstyle editing capability. If set to true, we\nuse a simplified getline input with limited\nediting capability. Defaults to false.", false, &bridge.useGetline);
    popl::Value<int> maxhistoryOption("x", "maxhistory", "Integer (nonnegative). The maximum number of\nlines to keep in the input history.Defaults\nto 10.", 10);

    popl::OptionParser op("MathLine Usage");
    op.add(helpOption)
            .add(checkOption)
            .add(mainloopOption)
            .add(promptOption)
            .add(iostringsOption)
            .add(linknameOption)
            .add(linkmodeOption)
            .add(getlineOption)
            .add(maxhistoryOption);

    // Parse the options.
    try{
        op.parse(argc, argv);
    }catch (std::invalid_argument &e){
        std::cout << "Error: " << e.what() << ".\n";
        std::cout << op << std::endl;
        return QUIT_WITH_ERROR;
    };

    //Check for unknown options.
    if( !op.unknownOptions().empty()) {
        for(const auto &n : op.unknownOptions())
            std::cout << "Unknown option: " << n << "\n";
        std::cout << op << std::endl;
        return QUIT_WITH_ERROR;
    }
    //Print help message and exit.
    if ( helpOption.isSet() ){
        std::cout << op << std::endl;
        return QUIT_WITH_SUCCESS;
    }

    //Now apply the options to our MLBridge instance.
    //(We are able to apply some automatically above.)
    if(checkOption.isSet()){
        check_and_exit = true;
    }
    if(linknameOption.isSet()){
        std::string str = linknameOption.getValue();
        if(str.empty()){
            //Empty string. Ignore this option.
            std::cout << "Option linkname cannot be empty. Ignoring." << std::endl;
        } else{
            //Make a copy, because linknameOption will go out of scope and free the linkname before we can connect with it.
            bridge.argv[3] = copyDataFromString(str);
        }
    }
    if(linkmodeOption.isSet()){
        std::string str = linkmodeOption.getValue();
        if(str.empty()){
            //Empty string. Ignore this option.
            std::cout << "Option linkmode cannot be empty. Ignoring." << std::endl;
        } else{
            //Make a copy, because linkmodeOption will go out of scope and free the linkmode before we can connect with it.
            bridge.argv[1] = copyDataFromString("-" + str);
        }
    }
    if(maxhistoryOption.isSet()){
        int max = maxhistoryOption.getValue();
        if (max >= 0) {
            bridge.SetMaxHistory(max);
        } else{
            std::cout << "Option maxhistory must be nonnegative. Ignoring." << std::endl;
        }
    }

    return CONTINUE;
}

int main(int argc, const char * argv[]) {
    //Banner
    std::cout << "MathLine v" MATHLINE_VERSION ": A free and open source textual interface to Mathematica." << std::endl;

    MLBridge bridge;
    
    //Parse the command line arguments.
    int parseFailed = ParseProgramOptions(bridge, argc, argv);
    if (CONTINUE != parseFailed) {
        // An unknown argument was supplied, so we exit.
        return parseFailed;
    }
    
    //Attempt to establish the MathLink connection using the options we've set.
    try{
        bridge.Connect();
    } catch(MLBridgeException &e){
        std::cerr << e.ToString() << "\n";
        std::cerr << "Could not connect to Mathematica. Check that " << bridge.argv[3] << " works from a command line." << std::endl;
        return 1;
    }
    if(bridge.IsConnected()){
        //Let's print the kernel version.
        std::cout << "Mathematica " << bridge.GetKernelVersion() << "\n" << std::endl;
        if( check_and_exit ){
            //Don't enter the REPL, just check and exit.
            std::string test = "1+2";
            std::cout << bridge.kernelPrompt << test << "\n";
            std::cout << bridge.GetEvaluated("1+2") << std::endl;
        }else{
            bridge.REPL();
        }

    } else{
        std::cout << "MLBridge failed to connect.";
    }
        
    return 0;
}

#pragma clang diagnostic pop
