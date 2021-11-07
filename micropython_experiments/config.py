import json

def load(fname):
    with open(fname, 'r') as fp:
        return json.load(fp)

def save(config, fname):
    with open(fname, 'w') as fp:
        json.dump(config, fp)

