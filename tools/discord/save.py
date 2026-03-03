import hashlib
import os
from typing import Optional


async def save_to_folder(*, ghost: Optional[bytes], krkg: Optional[bytes], folder_name: str):
    folder_path = os.path.join(os.getcwd(), folder_name)

    try:
        os.makedirs(folder_path, exist_ok=True)
    except OSError as e:
        print(f"Error creating directory: {e}")

    # Filename based off MD5 of file contents
    ghost_file_name = hashlib.md5(ghost).hexdigest() if ghost else None

    if ghost:
        file_path = os.path.join(folder_path, ghost_file_name + ".rkg")
        try:
            with open(file_path, "w+b") as f:
                f.write(ghost)
        except OSError as e:
            print(f"Error writing out file to {file_path}")

    if krkg:
        file_path = os.path.join(folder_path, ghost_file_name + ".krkg")
        try:
            with open(file_path, "w+b") as f:
                f.write(ghost)
        except OSError as e:
            print(f"Error writing out file to {file_path}")
