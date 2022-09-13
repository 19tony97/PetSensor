from datetime import datetime
from socket import socket, AF_INET, SOCK_STREAM
from ssl import SSLContext, PROTOCOL_TLS_SERVER

from Crypto.Cipher import AES

import smtplib
import time
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText


ip = '0.0.0.0'
port = 8090
context = SSLContext(PROTOCOL_TLS_SERVER)
context.load_cert_chain('cert.pem', 'key.pem')
key = 'abcdefghijklmnop'

# A caso
temperature_baseline = 25
last_email_time = 0


def decode_text(text):
	cipher_text = bytearray.fromhex(text)
	decrypt_text = cipher.decrypt(bytes(cipher_text))
	decrypt_text_without_spaces = decrypt_text.decode("utf-8").replace(" ", "").replace('\x00', "")
	return decrypt_text_without_spaces


def get_contacts(filename):
	names = []
	emails = []
	with open(filename, mode='r', encoding='utf-8') as contacts_file:
		for a_contact in contacts_file:
			names.append(a_contact.split()[0])
			emails.append(a_contact.split()[1])
	return names, emails


def read_template(filename):
	with open(filename, 'r', encoding='utf-8') as template_file:
		template_file_content = template_file.read()
	return template_file_content


def send_email(values):
	s = smtplib.SMTP(host='smtp-mail.outlook.com', port=587)
	s.starttls()
	s.login('MessageAlert@outlook.it', 'esamesicurezza123')

	names, emails = get_contacts('contacts.txt')
	message_template = read_template('message.txt')

	for name, email in zip(names, emails):
		msg = MIMEMultipart()
		
		message = message_template.format(name = name, temperature=values[0]+values[1])
		# setup the parameters of the message
		msg['From'] = 'MessageAlert@outlook.it'
		msg['To'] = email
		msg['Subject'] = "Warning"

		msg.attach(MIMEText(message, 'plain'))
		s.send_message(msg)

		print('Email sent')
		del msg


with socket(AF_INET, SOCK_STREAM) as server:
	server.bind((ip, port))
	server.listen(1)
	with context.wrap_socket(server, server_side=True) as tls:
		while True:
			connection, address = tls.accept()
			encrypted = connection.recv(1024)
			cipher = AES.new(str.encode(key), AES.MODE_ECB)
			encrypted_decode = encrypted.decode("utf-8")
			numbers = encrypted_decode.split(" ")
			temperature = decode_text(numbers[0])
			print(
				f'[{datetime.today()}] Encrypted temperature: {numbers[0]}')
			print(
				f'[{datetime.today()}] Decrypted temperature: {temperature}')

			print()
			if int(temperature) >= temperature_baseline + 5:
				print('Anomalies detected in the measurements')

				if time.time() - last_email_time >= 120:
					send_email((temperature))
					last_email_time = time.time()
				else:
					print('Can\'t send another email before 2 minutes. Last email was sent {} seconds ago'
						.format(int(time.time() - last_email_time)))