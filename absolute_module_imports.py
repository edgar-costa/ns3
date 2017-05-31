import subprocess

def _locate_file(file_name):
    """
    locate_file
    """
    # subprocess.Popen(['sudo', 'updatedb']).wait()

    p = subprocess.Popen(['sudo', 'su', '--', 'edgar', '-c',
                         'locate -br "{0}"'.format(file_name)],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = p.communicate()

    return stdout, stderr


def get_file_path(file_name):

    paths, _ = _locate_file(file_name)
    path = paths.split("\n")[0]
    return path


def find_special_include(line, special_word):

    if line.startswith("#include"):
        if special_word in line:
            return True

    return False


def absolutize_includes(file_name, special_word):

    file_path = get_file_path(file_name)
    f = open(file_path, "r")
    file_content_lines = f.readlines()
    f.close()

    # open temporal file
    f = open(file_path+"_tmp", "w")
    for line in file_content_lines:
        if find_special_include(line, special_word):
            last_file = (line.split("/")[-1])[:-2]
            last_file_absolute_path = get_file_path(last_file)
            if MAC:
                last_file_absolute_path = last_file_absolute_path.replace("home", "Users")
                last_file_absolute_path = last_file_absolute_path.replace("ns-allinone-3.26/ns-3.26", "ns3")

            f.write('#include "'+last_file_absolute_path+'"\n')
        else:
            f.write(line)
    f.close()

    subprocess.call(['mv', file_path+"_tmp", file_path])
    # subprocess.call(['rm', file_path+"_tmp"])
    subprocess.call(['chown', 'edgar.edgar', file_path])
    subprocess.call(['chmod', '664', file_path])


def deabsolutize_includes(file_name, special_word):
    file_path = get_file_path(file_name)
    f = open(file_path, "r")
    file_content_lines = f.readlines()
    f.close()

    # open temporal file
    f = open(file_path+"_tmp", "w")
    for line in file_content_lines:
        if find_special_include(line, special_word):
            last_file = (line.split("/")[-1])[:-2]
            f.write('#include "ns3/'+last_file+'"\n')
        else:
            f.write(line)
    f.close()

    subprocess.call(['mv', file_path+"_tmp", file_path])
    # subprocess.call(['rm', file_path+"_tmp"])
    subprocess.call(['chown', 'edgar.edgar', file_path])
    subprocess.call(['chmod', '664', file_path])


if __name__ == "__main__":

    import sys
    MAC = True
    args = sys.argv[1:]
    try:
        file_name = args[0]
    except:
        print "Error: file name not given"
        sys.exit(1)
    try:
        action = args[1]
    except:
        print "Warning action not given. Default is absoulte"
        action = "absolute"
        
    if action == "absolute":
        absolutize_includes(file_name=, "ns3")
    elif action == "relative":
        deabsolutize_includes(file_name, "edgar")
