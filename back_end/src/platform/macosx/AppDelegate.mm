#import "AppDelegate.h"

#include "gui_fasto_application.h"

#include "network/network_controller.h"

@interface OnlyIntegerValueFormatter : NSNumberFormatter
@end

@implementation OnlyIntegerValueFormatter

- (BOOL)isPartialStringValid:(NSString*)partialString newEditingString:(NSString**)newString errorDescription:(NSString**)error
{
    if([partialString length] == 0) {
        return YES;
    }
    
    NSScanner* scanner = [NSScanner scannerWithString:partialString];
    
    if(!([scanner scanInt:0] && [scanner isAtEnd])) {
        NSBeep();
        return NO;
    }
    
    return YES;
}

@end

@interface  AppDelegate()
@property (nonatomic, assign) fasto::siteonyourdevice::application::MacMainWindow *cxx_window_;
@property (nonatomic, assign) fasto::siteonyourdevice::network::NetworkController *cxx_controller_;
@end

@implementation AppDelegate
@synthesize cxx_window_;
@synthesize cxx_controller_;

- (id)initWithCxxWindow: (void*) cxxWindow cxxController : (void*)controller
{    
    using namespace fasto::siteonyourdevice;
    if ( self = [super init] ) {
        cxx_window_ = (application::MacMainWindow *)cxxWindow;
        cxx_controller_ = (network::NetworkController *)controller;
        // create a reference rect
        NSRect contentSize = NSMakeRect(0.0f, 0.0f, application::MacMainWindow::width, application::MacMainWindow::height);
        
        // allocate window
        window = [[NSWindow alloc] initWithContentRect:contentSize styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask backing:NSBackingStoreBuffered defer:YES];
        [window center];
        
        const int control_heigth = 25;
        const int text_control_heigth = 20;
        const int width_control = 65;
        const int padding = 10;
        const int left_padding = padding;
        const int buttom_padding = padding;
        const int top_padding = application::MacMainWindow::height - padding - control_heigth;

        const int center_widget_width = application::MacMainWindow::width / 2;
        const int center_widget_height = application::MacMainWindow::height / 2;
        
        NSRect domainFrameLabel = NSMakeRect(left_padding, top_padding, width_control, control_heigth);
        domainLabel_ = [[NSTextField alloc] initWithFrame:domainFrameLabel];
        
        NSRect domainFrameText = NSMakeRect(left_padding + width_control + padding, top_padding + (control_heigth - text_control_heigth),
                                            width_control, text_control_heigth);
        domainTextBox_ = [[NSTextField alloc] initWithFrame:domainFrameText];
        
        NSRect portFrameLabel = NSMakeRect(center_widget_width, top_padding, width_control, control_heigth);
        portLabel_ = [[NSTextField alloc] initWithFrame:portFrameLabel];
        
        NSRect portFrameText = NSMakeRect(center_widget_width + width_control + padding, top_padding + (control_heigth - text_control_heigth),
                                            width_control, text_control_heigth);
        portTextBox_ = [[NSTextField alloc] initWithFrame:portFrameText];
        
        const int top_padding_2 = top_padding - control_heigth;
        
        NSRect loginFrameLabel = NSMakeRect(left_padding, top_padding_2, width_control, control_heigth);
        loginLabel_ = [[NSTextField alloc] initWithFrame:loginFrameLabel];
        
        NSRect loginFrameText = NSMakeRect(left_padding + width_control + padding, top_padding_2 + (control_heigth - text_control_heigth),
                                            width_control, text_control_heigth);
        loginTextBox_ = [[NSTextField alloc] initWithFrame:loginFrameText];        
        
        const int top_padding_3 = top_padding_2 - control_heigth;
        
        NSRect passwordFrameLabel = NSMakeRect(left_padding, top_padding_3, width_control, control_heigth);
        passwordLabel_ = [[NSTextField alloc] initWithFrame:passwordFrameLabel];
        
        NSRect passwordFrameText = NSMakeRect(left_padding + width_control + padding, top_padding_3 + (control_heigth - text_control_heigth),
                                            width_control, text_control_heigth);
        passwordTextBox_ = [[NSSecureTextField alloc] initWithFrame:passwordFrameText];
        
        
        const int top_padding_4 = top_padding_3 - control_heigth;
        
        NSRect contentPathFrameLabel = NSMakeRect(left_padding, top_padding_4 - control_heigth, width_control, control_heigth * 2);
        contentPathLabel_ = [[NSTextField alloc] initWithFrame:contentPathFrameLabel];
        
        NSRect contentPathFrameText = NSMakeRect(left_padding + width_control + padding, top_padding_4 + (control_heigth - text_control_heigth),
                                              width_control, text_control_heigth);
        contentPathTextBox_ = [[NSTextField alloc] initWithFrame:contentPathFrameText];
        
        NSRect selectFrame = NSMakeRect(center_widget_width, top_padding_4, width_control - 25, control_heigth);
        selectPathButton_ = [[NSButton alloc] initWithFrame: selectFrame];

        const int top_padding_5 = top_padding_4 - control_heigth * 2;

        NSRect exFrame = NSMakeRect(left_padding, top_padding_5, width_control + 35, control_heigth);
        isExternalSiteCheckBox_ = [[NSButton alloc] initWithFrame: exFrame];

        const int left_padding_x2 = left_padding + width_control + 35 + padding;

        NSRect extHostFrame = NSMakeRect(left_padding_x2, top_padding_5,
                                            width_control, text_control_heigth);
        externalHostTextBox_ = [[NSTextField alloc] initWithFrame:extHostFrame];

        const int left_padding_x3 = left_padding_x2 + width_control + padding;
        NSRect extPortFrame = NSMakeRect(left_padding_x3, top_padding_5,
                                            width_control, text_control_heigth);
        externalPortTextBox_ = [[NSTextField alloc] initWithFrame:extPortFrame];

        const int top_padding_6 = top_padding_5 - control_heigth * 2;

        NSRect privateFrame = NSMakeRect(left_padding, top_padding_6, width_control + 35, control_heigth);
        isPrivateSiteCheckBox_ = [[NSButton alloc] initWithFrame: privateFrame];

        NSRect frame = NSMakeRect(center_widget_width - (left_padding + width_control/2), buttom_padding, width_control + 35, control_heigth);
        connetButton_ = [[NSButton alloc] initWithFrame: frame];

        // allocate view
        view = [[NSView alloc] initWithFrame:contentSize];
    }
    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{    
    // attach the view to the window
    [window setContentView:view];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // make the window visible.
    NSApplication *app = [notification object];
    NSMenu *appleMenu = [[NSMenu alloc] initWithTitle: @"Apple Menu"];
    [appleMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    NSMenuItem * menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu *mainMenu = [[NSMenu alloc] initWithTitle: @""];
    [menuItem setSubmenu:appleMenu];
    [mainMenu addItem:menuItem];
    [app setMainMenu:mainMenu];
    [app setAppleMenu:appleMenu];
    [menuItem release];
    [mainMenu release];
    [appleMenu release];
    
    using namespace fasto::siteonyourdevice;

    HttpConfig config = cxx_controller_->config();
    
    [domainLabel_ setStringValue:@ DOMAIN_LABEL];
    [domainLabel_ setBezeled:NO];
    [domainLabel_ setDrawsBackground:NO];
    [domainLabel_ setEditable:NO];
    [domainLabel_ setSelectable:NO];
    [window.contentView addSubview:domainLabel_];
    
    NSString *loc_host = [NSString stringWithCString:config.local_host_.host_.c_str()
                                                encoding:[NSString defaultCStringEncoding]];
    [domainTextBox_ setStringValue: loc_host];
    [window.contentView addSubview:domainTextBox_];
    
    [portLabel_ setStringValue:@ PORT_LABEL];
    [portLabel_ setBezeled:NO];
    [portLabel_ setDrawsBackground:NO];
    [portLabel_ setEditable:NO];
    [portLabel_ setSelectable:NO];
    [window.contentView addSubview:portLabel_];
    
    const uint16 loc_port = config.local_host_.port_;
    [portTextBox_ setIntValue: loc_port];
    OnlyIntegerValueFormatter * formatter = [[OnlyIntegerValueFormatter alloc] init];
    [portTextBox_ setFormatter: formatter];
    [window.contentView addSubview:portTextBox_];
    
    [loginLabel_ setStringValue:@ LOGIN_LABEL];
    [loginLabel_ setBezeled:NO];
    [loginLabel_ setDrawsBackground:NO];
    [loginLabel_ setEditable:NO];
    [loginLabel_ setSelectable:NO];
    [window.contentView addSubview:loginLabel_];
    
    NSString *login = [NSString stringWithCString:config.login_.c_str()
                                          encoding:[NSString defaultCStringEncoding]];
    [loginTextBox_ setStringValue: login];
    [window.contentView addSubview:loginTextBox_];
    
    [passwordLabel_ setStringValue:@ PASSWORD_LABEL];
    [passwordLabel_ setBezeled:NO];
    [passwordLabel_ setDrawsBackground:NO];
    [passwordLabel_ setEditable:NO];
    [passwordLabel_ setSelectable:NO];
    [window.contentView addSubview:passwordLabel_];
    
    NSString *password = [NSString stringWithCString:config.password_.c_str()
                                         encoding:[NSString defaultCStringEncoding]];
    [passwordTextBox_ setStringValue: password];
    [window.contentView addSubview:passwordTextBox_];
    
    [contentPathLabel_ setStringValue:@ CONTENT_PATH_LABEL];
    [contentPathLabel_ setBezeled:NO];
    [contentPathLabel_ setDrawsBackground:NO];
    [contentPathLabel_ setEditable:NO];
    [contentPathLabel_ setSelectable:NO];
    [window.contentView addSubview:contentPathLabel_];
    
    NSString *cpath = [NSString stringWithCString:config.content_path_.c_str()
                                            encoding:[NSString defaultCStringEncoding]];
    [contentPathTextBox_ setStringValue: cpath];
    [window.contentView addSubview:contentPathTextBox_];
    
    selectPathButton_.bezelStyle = NSRoundedBezelStyle;
    [selectPathButton_ setTitle:@ "..."];
    [selectPathButton_ setAction:@selector(selectContentPathAction:)];
    [window.contentView addSubview:selectPathButton_];

    [isExternalSiteCheckBox_ setButtonType:NSSwitchButton];
    [isExternalSiteCheckBox_ setTitle:@ EXTERNAL_SITE_LABEL];
    [isExternalSiteCheckBox_ setState:config.server_type_ == EXTERNAL_SERVER ? NSOnState : NSOffState];
    [isExternalSiteCheckBox_ setAction:@selector(externalSiteChangeAction:)];
    [window.contentView addSubview:isExternalSiteCheckBox_];

    NSString *ex_domain = [NSString stringWithCString:config.external_host_.host_.c_str()
                                                encoding:[NSString defaultCStringEncoding]];
    [externalHostTextBox_ setStringValue: ex_domain];
    [window.contentView addSubview:externalHostTextBox_];

    const uint16 ex_port = config.external_host_.port_;
    [externalPortTextBox_ setIntValue: ex_port];
    [externalPortTextBox_ setFormatter: formatter];
    [window.contentView addSubview:externalPortTextBox_];

    [isPrivateSiteCheckBox_ setButtonType:NSSwitchButton];
    [isPrivateSiteCheckBox_ setTitle:@ PRIVATE_SITE_LABEL];
    [isPrivateSiteCheckBox_ setState:config.is_private_site_ ? NSOnState : NSOffState];
    [self externalSiteChangeAction: nil];
    [window.contentView addSubview:isPrivateSiteCheckBox_];

    //[view setWantsLayer:YES];
    connetButton_.bezelStyle = NSRoundedBezelStyle;
    [connetButton_ setTitle:@ CONNECT_LABEL];
    [connetButton_ setAction:@selector(connectAction:)];
    [window.contentView addSubview:connetButton_];
    
    [window makeKeyAndOrderFront:self];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    cxx_window_->onExit();
}

- (IBAction) connectAction : (id) sender;
{
    using namespace fasto::siteonyourdevice;

    HttpConfig config = cxx_controller_->config();
    NSString *loc_domain = [domainTextBox_ stringValue];
    const std::string loc_domain_str = [loc_domain UTF8String];
    int loc_port = [portTextBox_ intValue];
    config.local_host_ = common::net::hostAndPort(loc_domain_str, loc_port);

    NSString *login = [loginTextBox_ stringValue];
    config.login_ = [login UTF8String];
    NSString *password = [passwordTextBox_ stringValue];
    config.password_ = [password UTF8String];
    NSString *contentPath = [contentPathTextBox_ stringValue];
    config.content_path_ = [contentPath UTF8String];

    config.is_private_site_ = [isPrivateSiteCheckBox_ state] == NSOnState;
    config.server_type_ = [isExternalSiteCheckBox_ state] == NSOnState ? EXTERNAL_SERVER : FASTO_SERVER;

    NSString *ex_domain = [externalHostTextBox_ stringValue];
    const std::string ex_domain_str = [ex_domain UTF8String];
    int ex_port = [portTextBox_ intValue];
    config.external_host_ = common::net::hostAndPort(ex_domain_str, ex_port);

    cxx_window_->onConnectClicked(config);
}

- (IBAction) disConnectAction : (id) sender
{
    cxx_window_->onDisconnectClicked();
}

- (IBAction) externalSiteChangeAction : (id) sender
{
    bool isOn = [isExternalSiteCheckBox_ state] == NSOnState;
    if(isOn){
        [selectPathButton_ setEnabled:NO];
        [portTextBox_ setEnabled:NO];
        [contentPathTextBox_ setEnabled:NO];

        [externalHostTextBox_ setEnabled:YES];
        [externalPortTextBox_ setEnabled:YES];
    }
    else{
        [selectPathButton_ setEnabled:YES];
        [portTextBox_ setEnabled:YES];
        [contentPathTextBox_ setEnabled:YES];

        [externalHostTextBox_ setEnabled:NO];
        [externalPortTextBox_ setEnabled:NO];
    }
}

- (IBAction) selectContentPathAction : (id) sender
{
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    // Enable the selection of files in the dialog.
    [openDlg setCanChooseFiles:NO];

    // Multiple files not allowed
    [openDlg setAllowsMultipleSelection:NO];

    // Can't select a directory
    [openDlg setCanChooseDirectories:YES];

    NSString *contentPath = [contentPathTextBox_ stringValue];
    NSURL *url = [NSURL URLWithString:contentPath];
    [openDlg setDirectoryURL: url];
    
    // Display the dialog. If the OK button was pressed,
    // process the files.
    if ( [openDlg runModal] == NSOKButton )
    {
        NSURL *selectedUrl = [openDlg directoryURL];
        [contentPathTextBox_ setStringValue: [selectedUrl path]];
    }
}

- (void) handleNetworkEvent: (void*) revent
{
     using namespace fasto::siteonyourdevice;
     network::NetworkEvent* event = reinterpret_cast<network::NetworkEvent*>(revent);
     if(event->eventType() == network::InnerClientConnectedEvent::EventType){
         [connetButton_ setTitle:@ DISCONNECT_LABEL];
         [connetButton_ setAction:@selector(disConnectAction:)];
     }
     else if(event->eventType() == network::InnerClientDisconnectedEvent::EventType){
         [connetButton_ setTitle:@ CONNECT_LABEL];
         [connetButton_ setAction:@selector(connectAction:)];
     }
     else{

     }
}

- (void)dealloc
{
    // don’t forget to release allocated objects!
    [view release];
    [window release];
    
    [super dealloc];
}

@end
