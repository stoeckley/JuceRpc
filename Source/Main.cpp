/*
  Copyright 2016 Art & Logic Software Development. 
 */



#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"

#include "Controller.h"
#include "RpcClient.h"
#include "RpcException.h"
#include "RpcServer.h"
#include "RpcTest.h"


#include "UI/ModeSelect.h"




//==============================================================================
class RpcTestApplication  : public JUCEApplication
{
public:
    //==============================================================================
    RpcTestApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

        UnitTestRunner testRunner;

        testRunner.runAllTests();
     
        ScopedPointer<Component> c(new ModeSelect());
        int retval = DialogWindow::showModalDialog("Select Mode", c, nullptr, Colours::grey, false);
        mainWindow = new MainWindow (getApplicationName());

        if (0 == retval)
        {
            // server mode...
            mainWindow->SetText("SERVER");
            ServerController* c = new ServerController();
            mainWindow->SetController(c);
            fRpcServer = new RpcServer(c);
            this->RunServer();
        }
        else
        {
            // client mode.
            mainWindow->SetText("client");
            RpcClient* ipc = new RpcClient();
            fClientController = new ClientController(ipc);
            mainWindow->SetController(fClientController);
            this->RunClient();
        }


    }


    void RunServer()
    {
        DBG("Launching Server thread.");
#ifdef qUseNamedPipe
        fRpcServer->
#else        
        fRpcServer->beginWaitingForSocket(kPortNumber);
#endif    
    }


    void RunClient()
    {
        String hostname("127.0.0.1");

        if (fClientController->ConnectToServer(hostname, kPortNumber, 1500))
        {
            fClientController->VoidFn();
            
            int intVal = fClientController->IntFn(200);
            DBG("IntFn returned " + String(intVal));

            try
            {
                fClientController->IntFn(-1);
            }
            catch (const RpcException& e)
            {
                DBG("Got (expected) exception calling IntFn(-1);");
                jassert(Controller::kParameterError == e.GetCode());
            }
            catch (...)
            {
                DBG("Shouldn't get here!");
            }

            try
            {
                fClientController->UnknownFn();
            }
            catch (const RpcException& e)
            {
                DBG("Got (expected) exception calling UnknownFn());");
                jassert(Controller::kUnknownMethodError == e.GetCode());
            }
           
            
        }
        else
        {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                "Connection Error",
                "Can't connect to server.",
                "OK"
                );
            this->quit();
        }
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..
        if (nullptr != fRpcServer)
        {
            fRpcServer->stop();
            fRpcServer = nullptr;
        }
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainContentComponent class.
    */
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (String name)  : DocumentWindow (name,
                                                    Colours::lightgrey,
                                                    DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainContentComponent(), true);

            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */
        
        void SetText(const String& txt)
        {
            MainContentComponent* c = dynamic_cast<MainContentComponent*>(this->getContentComponent());
            if (c)
            {
                c->SetText(txt);
            }
        }

        void SetTreeText(const String& txt)
        {
            MainContentComponent* c = dynamic_cast<MainContentComponent*>(this->getContentComponent());
            if (c)
            {
                c->SetTreeText(txt);
            }
        }

        void SetController(Controller* controller)
        {
            MainContentComponent* c = dynamic_cast<MainContentComponent*>(this->getContentComponent());
            if (c)
            {
                c->SetController(controller);
            }
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    ScopedPointer<MainWindow> mainWindow;

    // if we're operating in client mode.
    ScopedPointer<ClientController> fClientController;

    // if we're running in server mode.
    ScopedPointer<RpcServer>    fRpcServer;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (RpcTestApplication)
