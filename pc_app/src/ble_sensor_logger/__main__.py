"""Command entrypoint."""

from __future__ import annotations

import argparse
import asyncio

from .app_core import SensorLoggerApp
from .ui_shell import InteractiveShell
from .web_api import run_web_server


async def _run_once(args: argparse.Namespace) -> None:
    app = SensorLoggerApp()
    match args.command:
        case "scan":
            for device in await app.scan():
                print(f"{device.name} {device.address}")
        case _:
            raise SystemExit(f"unsupported --once command: {args.command}")


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--once", action="store_true", help="run a single command and exit")
    parser.add_argument("--web", action="store_true", help="run the local WebGUI server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("command", nargs="?", default=None)
    return parser.parse_args(argv)


async def _async_main(args: argparse.Namespace) -> None:
    if args.once:
        await _run_once(args)
        return

    await InteractiveShell().run()


def main(argv: list[str] | None = None) -> None:
    args = _parse_args(argv)
    if args.web:
        run_web_server(args.host, args.port)
        return
    asyncio.run(_async_main(args))


if __name__ == "__main__":
    main()
