import phonenumbers
from phonenumbers import geocoder, carrier, timezone
import tkinter as tk
from tkinter import messagebox
import webbrowser
from geopy.geocoders import Nominatim

def trace_number():
    target = entry.get()
    try:
        p = phonenumbers.parse(target, None)

        if not phonenumbers.is_valid_number(p):
            messagebox.showerror("Invalid", "The phone number is not valid.")
            return

        location = geocoder.description_for_number(p, "en")
        provider = carrier.name_for_number(p, "en")
        zones = timezone.time_zones_for_number(p)
        formatted = phonenumbers.format_number(p, phonenumbers.PhoneNumberFormat.INTERNATIONAL)

        result_text = f"""
Formatted: {formatted}
Location: {location}
Carrier: {provider if provider else 'N/A'}
Time Zone(s): {', '.join(zones)}
        """.strip()

        output.config(state="normal")
        output.delete("1.0", tk.END)
        output.insert(tk.END, result_text)
        output.config(state="disabled")

        # Save location globally for map use
        global traced_location
        traced_location = location

    except phonenumbers.NumberParseException:
        messagebox.showerror("Error", "Could not parse the number.")

def open_map():
    if not traced_location:
        messagebox.showinfo("Info", "Trace a number first.")
        return
    geolocator = Nominatim(user_agent="phone_tracer")
    location = geolocator.geocode(traced_location)
    if location:
        lat, lon = location.latitude, location.longitude
        url = f"https://www.google.com/maps?q={lat},{lon}"
        webbrowser.open(url)
    else:
        messagebox.showerror("Error", "Could not locate region on the map.")

# GUI setup
root = tk.Tk()
root.title("PhoneTracer v2.1")
root.geometry("400x350")

tk.Label(root, text="Enter Phone Number (+254...)").pack(pady=10)
entry = tk.Entry(root, width=30)
entry.pack()

tk.Button(root, text="Trace", command=trace_number).pack(pady=10)
tk.Button(root, text="Open in Google Maps", command=open_map).pack(pady=5)

output = tk.Text(root, height=8, width=50, state="disabled", wrap="word")
output.pack(pady=10)

traced_location = None  # Global location tracker

root.mainloop()
