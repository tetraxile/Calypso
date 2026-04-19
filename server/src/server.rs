use std::{borrow::Cow, future::pending};

use futures_util::future::Either;
use tokio::{
	net::{TcpListener, TcpStream, UdpSocket},
	select,
	sync::mpsc,
};
use tracing::{error, warn};

pub enum ToServer {}
pub enum ToUi {
	Log(String),
}

pub async fn server_task(
	ui: mpsc::UnboundedSender<ToUi>,
	_server: mpsc::UnboundedReceiver<ToServer>,
) {
	let tcp_listener = TcpListener::bind("0.0.0.0:8171")
		.await
		.expect("failed to start tcp server on 8171");
	let _udp = UdpSocket::bind("0.0.0.0:8171")
		.await
		.expect("failed to bind udp server on 8171");

	let send_log = |value: Cow<'_, str>| {
		ui.send(ToUi::Log(value.into_owned()))
			.expect("channel closed");
	};

	let mut client: Option<TcpStream> = None;

	let mut tcp_buffer = [0u8; 1024];
	loop {
		select! {
		  result = tcp_listener.accept() => {
			match result {
			  Ok((stream, _addr)) => client.replace(stream),
			  Err(error) => {
				error!("failed to accept client: {error}");
				continue
			  }
			};
		  }
		  result = client
			.as_ref()
			.map(|client| Either::Left(client.readable()))
			.unwrap_or_else(|| Either::Right(pending())) => {
			if result.is_err() {
			  client = None;
			  continue;
			}

			match client.as_mut().unwrap().try_read(&mut tcp_buffer) {
			  Ok(n) => {
				let Ok(message) = str::from_utf8(&mut tcp_buffer[..n]) else {
				  warn!("switch sent non-utf8 log message");
				  continue
				};
				send_log(message.into());
			  }
			  Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock => {},
			  Err(e) => {
				error!("failed to read, closing client {e}");
				client = None;
			  }
			}
		  }
		}
	}
}
