//
// Created by nray on 4/16/23.
// Mostly taken from https://github.com/rppicomidi/pico-embedded-cli

#include <stdio.h>
#include "pico/stdlib.h"
#include "embedded_cli.h"

static bool exitFlag = false;

static void onCommand(const char* name, char *tokens)
{
    printf("Received command: %s\r\n",name);

    for (int i = 0; i < embeddedCliGetTokenCount(tokens); ++i) {
        printf("Arg %d : %s\r\n", i, embeddedCliGetToken(tokens, i + 1));
    }
}

static void onExit(EmbeddedCli *cli, char *args, void *context)
{
    (void)cli;
    (void)args;
    (void)context;
    exitFlag = true;
    printf("Cli will shutdown now...\r\n");
}

static void onHello(EmbeddedCli *cli, char *args, void *context)
{
    (void)cli;
    printf("Hello, ");
    if (embeddedCliGetTokenCount(args) == 0)
        printf("%s", (const char *) context);
    else
        printf("%s", embeddedCliGetToken(args, 1));
    printf("\r\n");
}

static void onCommandFn(EmbeddedCli *embeddedCli, CliCommand *command)
{
    (void)embeddedCli;
    embeddedCliTokenizeArgs(command->args);
    onCommand(command->name == NULL ? "" : command->name, command->args);
}

static void writeCharFn(EmbeddedCli *embeddedCli, char c)
{
    (void)embeddedCli;
    putchar(c);
}

void cli_main()
{
    // Initialize the console
    stdio_init_all();
    printf("guisep_pi setting up CLI...\r\n");
    while(getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
        // flush out the console input buffer
    }

    // Initialize the CLI
    EmbeddedCli *cli = embeddedCliNewDefault();
    cli->onCommand = onCommandFn;
    cli->writeChar = writeCharFn;
    CliCommandBinding exitBinding = {
            "exit",
            "Stop CLI and exit",
            false,
            NULL,
            onExit
    };
    embeddedCliAddBinding(cli, exitBinding);

    CliCommandBinding helloBinding = {
            "hello",
            "Print hello message",
            true,
            (void *) "World",
            onHello
    };
    embeddedCliAddBinding(cli, helloBinding);

    printf("Cli is running. Type \"exit\" to exit\r\n");
    printf("Type \"help\" for a list of commands\r\n");
    printf("Use backspace and tab to remove chars and autocomplete\r\n");
    printf("Use up and down arrows to recall previous commands\r\n");

    embeddedCliProcess(cli);
    while (!exitFlag) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            embeddedCliReceiveChar(cli, c);
            embeddedCliProcess(cli);
        }
        else{
            printf("Hit PICO_ERROR_TIMEOUT, closing CLI...");
            exitFlag = true;
        }
    }
}