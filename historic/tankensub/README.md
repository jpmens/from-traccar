

## Install

```
$ virtualenv -p python3 py3
$ source py3/bin/activate
(py3) $ pip install -r requirements.txt
```

## Example

```
2018-09-11 17:20:27.464191 R=  _traccar/q54/event/ignitionOff u09tg6bf {
    "nodeid": "993642116",
    "lat": 48.8387525,
    "lon": 2.2535353,
    "ghash": "u09tg6bf",
    "addr": "12 Avenue de la Porte de Saint-Cloud, 75016 Paris, France",                                "tags": {
        "name": "Total Porte Saint-Cloud",
        "shop": "convenience",
        "source": "survey",
        "amenity": "fuel",
        "toilets": "yes",
        "vending": "fuel",
        "fuel:e10": "yes",
        "fuel:diesel": "yes",
        "opening_hours": "24/7",
        "compressed_air": "yes",
        "fuel:octane_98": "yes",
        "vending_machine": "fuel",
        "ref:FR:prix-carburants": "75016017"
    }
}
```
