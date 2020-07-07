import asyncio
import json

async def tcp_echo_client(message):
    reader, writer = await asyncio.open_connection(
        '127.0.0.1', 9876)

    message = f"{len(message.encode()):03d}" + message + '\0'
    print(f'Send: {message!r}')
    writer.write(message.encode())

    data = await reader.read(100)
    recvd = f'Received: {data.decode()}'
    print(recvd)
    writer.close()
    return recvd

async def tcp_echo_client_sleepy(message):
    reader, writer = await asyncio.open_connection(
        '127.0.0.1', 9876)

    message = f"{len(message.encode()):03d}" + message + '\0'
    print(f'Send: {message!r}')
    writer.write(message.encode())

    await asyncio.sleep(1)

    data = await reader.read(100)
    recvd = f'Received: {data.decode()}'
    print(recvd)
    writer.close()
    return recvd


async def main():

    selstr = """
Hallo SE14, was wollt ihr tun?
    (1) Echo
    (2) Würfeln
    (3) Fibonacci
    (4) End
    """
    tasks = []
    while True:

        for task in tasks:
            if task.done():
                print("Menwhile completed: ", task.result())

        sel = input(selstr)

        while not sel.isdigit():
            sel = input("Zahlen bitte")

        selection = int(sel)

        selection = selection if selection < 5 else 4
        selection = selection if selection > 0 else 0

        if selection == 4:
            break

        if selection != 1:
            num = input("Wie viele dürfen es denn heute sein? ")

            while not num.isdigit():
                num = input("Zahlen bitte!")

            num = int(num)

            message = ""
            if selection == 2:
                message = json.dumps({
                    "function": "rand",
                    "arg": num
                })

            if selection == 3:
                message = json.dumps({
                    "function": "fib",
                    "arg": num
                })

            tasks.append(asyncio.create_task(tcp_echo_client_sleepy(message)))

        else:
            message = input("Echo: ")
            tasks.append(asyncio.create_task(tcp_echo_client(message)))

        # Some computation in main
        await asyncio.sleep(0.2)

    for task in tasks:
        await task


if __name__ == "__main__":
    asyncio.run(main())