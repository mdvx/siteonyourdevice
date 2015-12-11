#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject{
    NSWindow * window;
    NSView * view;
    
    NSTextField *domainLabel_;
    NSTextField *domainTextBox_;
    
    NSTextField *portLabel_;
    NSTextField *portTextBox_;
    
    NSTextField *loginLabel_;
    NSTextField *loginTextBox_;
    
    NSTextField *passwordLabel_;
    NSSecureTextField *passwordTextBox_;
    
    NSTextField *contentPathLabel_;
    NSTextField *contentPathTextBox_;
    
    NSButton* selectPathButton_;

    NSButton* isPrivateSiteCheckBox_;
    NSButton* connetButton_;
}

- (id)initWithCxxWindow: (void*) cxxWindow cxxController : (void*)controller;
- (void)applicationWillFinishLaunching:(NSNotification *)notification;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (void)applicationWillTerminate:(NSNotification *)notification;
- (void)dealloc;

- (IBAction) connectAction : (id) sender;
- (IBAction) disConnectAction : (id) sender;
- (IBAction) selectContentPathAction : (id) sender;

- (void) handleNetworkEvent: (void*) event;

@end
