import os
import shutil
import subprocess
import sys


def main():
    if len(sys.argv) < 4:
        print("usage: <glsl_compiler_path> <src_shader_dir> <dst_shader_dir>")
        return 1

    glsl_compiler = sys.argv[1]
    src_dir = sys.argv[2]
    dst_dir = sys.argv[3]

    if not os.path.isfile(glsl_compiler):
        print("invalid glsl compiler path: {}".format(glsl_compiler))
        return 1

    if not os.path.isdir(src_dir):
        print("invalid source directory path: {}".format(glsl_compiler))
        return 1

    if not os.path.isdir(dst_dir):
        print("creating destination dir: {}".format(dst_dir))
        os.makedirs(dst_dir)

    for item in os.listdir(src_dir):
        src_path = os.path.join(src_dir, item)

        if os.path.isfile(src_path):
            filename, file_extension = os.path.splitext(item)

            if file_extension == ".glsl":
                dst_path = os.path.join(dst_dir, "{}.spv".format(filename))
                print("{} --> {}".format(src_path, dst_path))
                try:
                    args = [glsl_compiler, src_path, "-o", dst_path]
                    result = subprocess.check_output(args)
                    if len(result) > 0:
                        print(result)
                except subprocess.CalledProcessError as compile_error:
                    print(compile_error.output)
                    return 1
            else:
                dst_path = os.path.join(dst_dir, item)
                print("{} --> {}".format(src_path, dst_path))
                shutil.copyfile(src_path, dst_path)

if __name__ == '__main__':
    sys.exit(main())