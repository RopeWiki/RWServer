using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Mail;
using System.Threading;
using System.Windows.Forms;

namespace RWHeartbeat
{
	public partial class Form1 : Form
	{
		public Form1()
		{
			InitializeComponent();

			StopHeartbeat();

			LoadSMTPKey();

			Application.DoEvents();

			CheckForAutostart();
		}

		#region UI events

		private void btnStart_Click(object sender, EventArgs e)
		{
			StartHeartbeat();
		}

		private void btnStop_Click(object sender, EventArgs e)
		{
			StopHeartbeat();
		}

		private void btnRestart_Click(object sender, EventArgs e)
		{
			RestartProcess();
		}

		private void txtTimerInterval_TextChanged(object sender, EventArgs e)
		{
			StopHeartbeat();
		}

		private void timer1_Tick(object sender, EventArgs e)
		{
			var url = txtRWurl.Text;
			var result = Ping(url);

			if (result)
			{
				txtLastHeartbeat.Text = DateTime.Now.ToString();
			}
			else
			{
				// a) see if rw.exe is running. If not, restart. If it is, reboot

				var rwProcess = GetRWProcess();

				if (rwProcess == null)
				{
					SendEmail("RW server app crashed - restarting");

					RestartProcess();
					return;
				}

				//email people that server failed heartbeat
				SendEmail("RW server failed heartbeat - rebooting server");

				StartRebootSequence();
			}
		}

		private void StartRebootSequence()
		{
			timer1.Stop();

			BackColor = System.Drawing.Color.LightCoral;

			Text = "REBOOTING - press Stop to cancel";
			
			Application.DoEvents();

			Thread.Sleep(5000);

			Application.DoEvents();

			RebootServer();
		}

		private void btnHeartbeat_Click(object sender, EventArgs e)
		{
			var url = txtRWurl.Text;
			var result = Ping(url);

			txtLastHeartbeat.Text = result
				? DateTime.Now.ToString()
				: "failed heartbeat";
		}

		private void btnTestEmail_Click(object sender, EventArgs e)
		{
			SendEmail("RW Test Email");
		}

		private void btnRebootServer_Click(object sender, EventArgs e)
		{
			if (MessageBox.Show("Reboot server now?", "Reboot", MessageBoxButtons.YesNo) == DialogResult.Yes)
				RebootServer();
		}

		#endregion

		private void StartHeartbeat()
		{
			if (!int.TryParse(txtTimerInterval.Text, out var interval)) return;

			btnStart.Enabled = false;
			btnStop.Enabled = true;
			timer1.Interval = interval * 1000;
			timer1.Start();

			timer1_Tick(null, null);
		}

		private void StopHeartbeat()
		{
			timer1.Stop();

			btnStart.Enabled = true;
			btnStop.Enabled = false;
			BackColor = System.Drawing.SystemColors.Control;
			Text = "RW Heartbeat";
		}

		private bool Ping(string url)
		{
			try
			{
				var request = (HttpWebRequest) WebRequest.Create(url);
				request.Timeout = 3000;
				request.AllowAutoRedirect = false;
				request.Method = "POST";

				using (request.GetResponse())
				{
					return true;
				}
			}
			catch
			{
				return false;
			}
		}

		private void RestartProcess()
		{
			var rwProcess = GetRWProcess();

			if (rwProcess != null)
			{
				rwProcess.Kill();

				Thread.Sleep(3000);
			}

			var path = txtRWProcess.Text;
			var workingDirectory = Path.GetDirectoryName(path) ?? "";

			try
			{
				using (Process process = new Process())
				{
					process.StartInfo.FileName = path;
					process.StartInfo.WorkingDirectory = workingDirectory;
					process.Start();
				}
			}
			catch (Exception)
			{
				//email people that error occured
				var message = "RW server unable to restart - manual intervention needed";
				SendEmail(message);

				StopHeartbeat();
			}
		}

		private Process GetRWProcess()
		{
			var path = txtRWProcess.Text;

			var processName = Path.GetFileNameWithoutExtension(path);
			
			var processes = Process.GetProcessesByName(processName);

			return processes.Length > 0 ? processes[0] : null;
		}

		private void SendEmail(string subject)
		{
			var smtpHostUrl = "smtp.sendgrid.net";
			var username = "apikey";
			var password = txtSMTPKey.Text;

			SmtpClient smtpClient = new SmtpClient(smtpHostUrl, 587)
			{
				EnableSsl = true,
				Timeout = 10000,
				DeliveryMethod = SmtpDeliveryMethod.Network,
				UseDefaultCredentials = false,
				Credentials = new NetworkCredential(username, password)
			};

			MailMessage mailMsg = new MailMessage
			{
				From = new MailAddress("noreply@rwserver.com"),
				Subject = subject,
				Body = ""
			};

			var addresses =
				txtEmailRecipients.Text.Split(new[] {"\r\n", "\r", "\n"}, StringSplitOptions.RemoveEmptyEntries);

			foreach (var entry in addresses)
			{
				try
				{
					mailMsg.To.Add(entry);
				}
				catch (FormatException)
				{
				}
				catch (ArgumentException)
				{
				}
			}

			try
			{
				smtpClient.Send(mailMsg);
			}
			catch (InvalidOperationException)
			{
			}
		}

		private void LoadSMTPKey()
		{
			try
			{
				var lines = File.ReadAllLines(@"SMTPKey.txt");

				txtSMTPKey.Text = lines[0];

				for (int i = 1; i < lines.Length; i++)
				{
					txtEmailRecipients.Text += lines[i] + "\r\n";
				}
			}
			catch
			{
			}
		}

		private void CheckForAutostart()
		{
			//check if nginx is running
			var path = txtNginxProcess.Text;
			if (string.IsNullOrEmpty(path)) return;

			var processName = Path.GetFileNameWithoutExtension(path);

			var processes = Process.GetProcessesByName(processName);

			if (processes.Length > 0)
				return; //nginx already started

			//start nginx
			var startInfo = new ProcessStartInfo
			{
				WorkingDirectory = Path.GetDirectoryName(path) ?? "",
				FileName = processName
			};

			try
			{
				Process.Start(startInfo);
			}
			catch (Win32Exception)
			{
				MessageBox.Show("Invalid path for nginx");
				return;
			}

			Thread.Sleep(5000);

			//start rw server
			RestartProcess();

			Thread.Sleep(3000);

			StartHeartbeat();

			btnHeartbeat_Click(null, null);
			var succeeded = txtLastHeartbeat.Text != "failed heartbeat";

			//send email
			var message = succeeded
				? "RW server successfully rebooted and passed heartbeat"
				: "RW server failed heartbeat after reboot -- needs manual intervention";

			SendEmail(message);
		}

		private void RebootServer()
		{
			if (!btnStop.Enabled)
			{
				StopHeartbeat();
				return;
			}

			Process.Start("shutdown.exe", "-r -t 0");
		}
	}
}
