#!/usr/bin/python3
# Replication of several Clash for Windows "parser" commands.
# Credits to ChatGPT

import sys
import yaml

def parse_yaml_file(file_path):
    with open(file_path, 'r') as file:
        return yaml.safe_load(file)

def save_yaml_file(data, file_path):
    with open(file_path, 'w') as file:
        yaml.dump(data, file, default_flow_style=False, encoding="utf8", allow_unicode=True)

def run_parser(src_path, parser_path, url_string):
    # Parse the YAML files
    src = parse_yaml_file(src_path)
    parser = parse_yaml_file(parser_path)

    # Find the object with matching URL
    matching_parser = None
    for parser in parser['parsers']:
        if parser['url'] == url_string:
            matching_parser = parser
            break

    if not matching_parser:
        print(f"No matching parser found for URL: {url_string}")
        return

    # Process the matching parser's YAML object
    for action, props in matching_parser['yaml'].items():
        if action.startswith("append-"):
            prop_name = action.split("-", maxsplit=1)[1]
            if prop_name not in src:
                src[prop_name] = []
            src[prop_name].extend(props)
        elif action.startswith("prepend-"):
            prop_name = action.split("-", maxsplit=1)[1]
            if prop_name not in src:
                src[prop_name] = []
            src[prop_name] = props + src[prop_name]
        elif action.startswith("mix-"):
            prop_name = action.split("-", maxsplit=1)[1]
            if prop_name not in src:
                src[prop_name] = {}
            src[prop_name] |= props

    # Save the modified file
    save_yaml_file(src, src_path)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: script.py <src> <parser> <url_string>")
        sys.exit(1)

    src_path = sys.argv[1]
    parser_path = sys.argv[2]
    url_string = sys.argv[3]

    run_parser(src_path, parser_path, url_string)
