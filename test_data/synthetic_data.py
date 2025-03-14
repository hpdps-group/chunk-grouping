import random

from mkl_random.mklrand import randint

block_size = 8192
num_versions = 5
add_percent = 0.01
modify_percent = 0.1
delete_percent = 0.005

def split_blocks(data):
    return [data[i:i + block_size] for i in range(0, len(data), block_size)]

def add_new_blocks(blocks):
    num_add = int(len(blocks) * add_percent)
    for _ in range(num_add):
        # 模板块
        template_block = random.choice(blocks)
        # 从模板块中随机挑选字节构成新块
        new_block = bytearray(random.choice(template_block) for _ in range(block_size))
        add_pos = blocks.index(template_block) + 1
        blocks.insert(add_pos, new_block)
    return blocks

def modify_blocks(blocks):
    num_blocks_modify = int(len(blocks) * modify_percent)
    for _ in range(num_blocks_modify):
        # 随机选择块进行修改
        modify_block_index = random.randint(0, len(blocks) - 1)
        block = blocks[modify_block_index]
        # 随机选择修改长度
        modify_length = int(len(block) * random.uniform(0, 0.5))
        # 固定修改长度
        # modify_length = int(len(block) * 1)
        # 随机选择开始位置
        start_index = random.randint(0, len(block) - modify_length)
        modify_block = bytearray(block)
        for i in range(start_index, start_index + modify_length):
            modify_block[i] = random.choice(block)
        blocks[modify_block_index] = bytes(modify_block)
    return blocks

def delete_blocks(blocks):
    num_blocks_delete = int(len(blocks) * delete_percent)
    for _ in range(num_blocks_delete):
        delete_block_index = randint(0, len(blocks) - 1)
        del blocks[delete_block_index]
    return blocks

def gen_backup_version(data):
    blocks = split_blocks(data)
    backup_version = []
    for _ in range(num_versions):
        blocks = modify_blocks(blocks)
        blocks = add_new_blocks(blocks)
        #blocks = delete_blocks(blocks)
        backup_version.append(b"".join(blocks))
    return backup_version

if __name__ == "__main__":
    with open("/root/delta_compression_framework/test_data/synthetic_data2/17_Stage4bEpiData", "rb") as f:
        data = f.read()
    backup_versions = gen_backup_version(data)

    for i, version in enumerate(backup_versions):
        with open(f"/root/delta_compression_framework/test_data/synthetic_data2/backup_version_{i+1}.bin", "wb") as f:
            f.write(version)