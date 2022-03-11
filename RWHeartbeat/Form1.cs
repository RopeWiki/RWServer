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
        private readonly BackgroundWorker enterKeyWorker = new BackgroundWorker();

        public Form1()
        {
            InitializeComponent();

            enterKeyWorker.DoWork += enterKeyWorker_DoWork;

            StopHeartbeat();

            LoadSMTPKey();

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
                enterKeyWorker.RunWorkerAsync();

                if (MessageBox.Show("Reboot server?\r\n\r\nTimeout in 5 seconds", "Reboot imminent!",
                        MessageBoxButtons.YesNo) != DialogResult.Yes)
                    return;

                //email people that server failed heartbeat
                var message = "RW server failed heartbeat - rebooting server";
                SendEmail(message);

                timer1.Stop();

                Thread.Sleep(5000);

                RebootServer();
            }
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
            if (!Int32.TryParse(txtTimerInterval.Text, out var interval)) return;

            timer1_Tick(null, null);

            btnStart.Enabled = false;
            btnStop.Enabled = true;
            timer1.Interval = interval * 1000;
            timer1.Start();
        }

        private void StopHeartbeat()
        {
            btnStart.Enabled = true;
            btnStop.Enabled = false;
            timer1.Stop();
        }

        private bool Ping(string url)
        {
            try
            {
                var request = (HttpWebRequest)WebRequest.Create(url);
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
            var path = txtRWProcess.Text;

            var processName = Path.GetFileNameWithoutExtension(path);
            var workingDirectory = Path.GetDirectoryName(path);

            var processes = Process.GetProcessesByName(processName);

            if (processes.Length > 0)
            {
                var process = processes[0];
                process?.Kill();

                Thread.Sleep(3000);
            }

            try
            {
                using (Process process = new Process())
                {
                    process.StartInfo.FileName = path;
                    process.StartInfo.WorkingDirectory = workingDirectory ?? "";
                    process.Start();
                }
            }
            catch (Exception)
            {
                //email people that error occured
                var message = "!!!RW server unable to restart!!!";
                SendEmail(message);

                StopHeartbeat();
            }
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

            var addresses = txtEmailRecipients.Text.Split(new[] {"\r\n", "\r", "\n"}, StringSplitOptions.RemoveEmptyEntries);

            foreach (var entry in addresses)
            {
                try
                {
                    mailMsg.To.Add(entry);
                }
                catch (FormatException) { }
                catch (ArgumentException) { }
            }

            try
            {
                smtpClient.Send(mailMsg);
            }
            catch (InvalidOperationException) { }
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

            //ask for confirmation
            enterKeyWorker.RunWorkerAsync();

            if (MessageBox.Show("Autostart nginx?\r\n\r\nTimeout in 5 seconds", "Nginx process not found",
                    MessageBoxButtons.YesNo) != DialogResult.Yes)
                return;

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

        private void enterKeyWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            Thread.Sleep(5000);
            SendKeys.SendWait("{Enter}");//or Esc
        }

        private void RebootServer()
        {
            Process.Start("shutdown.exe", "-r -t 0");
        }
    }
}
