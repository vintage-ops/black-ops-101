import phonenumbers
from phonenumbers import geocoder
import time, random

def start_phone_tracer(target):
    print(f"[+] PhoneTracer v2.1 - OSINT")
    print(f"[*] Target: {target}")
    print(f"[*] Initializing trace ...")
    p = phonenumbers.parse(target, None)
    r = geocoder.description_for_number(p)
    print(f"[+] Location: {r}")
    print(f"[+] Trace complete")
start_phone_tracer("Input Phone number")
    