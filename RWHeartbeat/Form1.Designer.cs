namespace RWHeartbeat
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.btnStart = new System.Windows.Forms.Button();
            this.btnStop = new System.Windows.Forms.Button();
            this.btnRestart = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.txtRWProcess = new System.Windows.Forms.TextBox();
            this.txtEmailRecipients = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.txtRWurl = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.txtLastHeartbeat = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.txtTimerInterval = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtSMTPKey = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.btnTestEmail = new System.Windows.Forms.Button();
            this.btnHeartbeat = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnStart
            // 
            this.btnStart.Location = new System.Drawing.Point(72, 47);
            this.btnStart.Name = "btnStart";
            this.btnStart.Size = new System.Drawing.Size(137, 65);
            this.btnStart.TabIndex = 0;
            this.btnStart.Text = "Start";
            this.btnStart.UseVisualStyleBackColor = true;
            this.btnStart.Click += new System.EventHandler(this.btnStart_Click);
            // 
            // btnStop
            // 
            this.btnStop.Location = new System.Drawing.Point(237, 47);
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(137, 65);
            this.btnStop.TabIndex = 1;
            this.btnStop.Text = "Stop";
            this.btnStop.UseVisualStyleBackColor = true;
            this.btnStop.Click += new System.EventHandler(this.btnStop_Click);
            // 
            // btnRestart
            // 
            this.btnRestart.Location = new System.Drawing.Point(582, 248);
            this.btnRestart.Name = "btnRestart";
            this.btnRestart.Size = new System.Drawing.Size(101, 40);
            this.btnRestart.TabIndex = 2;
            this.btnRestart.Text = "Restart";
            this.btnRestart.UseVisualStyleBackColor = true;
            this.btnRestart.Click += new System.EventHandler(this.btnRestart_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(74, 253);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(135, 25);
            this.label1.TabIndex = 3;
            this.label1.Text = "RW process:";
            // 
            // txtRWProcess
            // 
            this.txtRWProcess.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtRWProcess.Location = new System.Drawing.Point(237, 250);
            this.txtRWProcess.Name = "txtRWProcess";
            this.txtRWProcess.Size = new System.Drawing.Size(339, 31);
            this.txtRWProcess.TabIndex = 4;
            this.txtRWProcess.Text = "C:\\rw\\RWR.exe";
            // 
            // txtEmailRecipients
            // 
            this.txtEmailRecipients.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtEmailRecipients.Location = new System.Drawing.Point(237, 400);
            this.txtEmailRecipients.Multiline = true;
            this.txtEmailRecipients.Name = "txtEmailRecipients";
            this.txtEmailRecipients.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtEmailRecipients.Size = new System.Drawing.Size(446, 128);
            this.txtEmailRecipients.TabIndex = 6;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(74, 403);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(104, 25);
            this.label2.TabIndex = 5;
            this.label2.Text = "Email list:";
            // 
            // timer1
            // 
            this.timer1.Interval = 30000;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // txtRWurl
            // 
            this.txtRWurl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtRWurl.Location = new System.Drawing.Point(237, 300);
            this.txtRWurl.Name = "txtRWurl";
            this.txtRWurl.Size = new System.Drawing.Size(446, 31);
            this.txtRWurl.TabIndex = 8;
            this.txtRWurl.Text = "http://luca.ropewiki.com/rwr?heartbeat";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(74, 303);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(83, 25);
            this.label3.TabIndex = 7;
            this.label3.Text = "RW url:";
            // 
            // txtLastHeartbeat
            // 
            this.txtLastHeartbeat.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtLastHeartbeat.Location = new System.Drawing.Point(237, 150);
            this.txtLastHeartbeat.Name = "txtLastHeartbeat";
            this.txtLastHeartbeat.ReadOnly = true;
            this.txtLastHeartbeat.Size = new System.Drawing.Size(339, 31);
            this.txtLastHeartbeat.TabIndex = 10;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(74, 153);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(156, 25);
            this.label4.TabIndex = 9;
            this.label4.Text = "Last heartbeat:";
            // 
            // txtTimerInterval
            // 
            this.txtTimerInterval.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtTimerInterval.Location = new System.Drawing.Point(237, 200);
            this.txtTimerInterval.Name = "txtTimerInterval";
            this.txtTimerInterval.Size = new System.Drawing.Size(446, 31);
            this.txtTimerInterval.TabIndex = 12;
            this.txtTimerInterval.Text = "30";
            this.txtTimerInterval.TextChanged += new System.EventHandler(this.txtTimerInterval_TextChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(74, 203);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(126, 25);
            this.label5.TabIndex = 11;
            this.label5.Text = "Timer (sec):";
            // 
            // txtSMTPKey
            // 
            this.txtSMTPKey.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtSMTPKey.Location = new System.Drawing.Point(237, 350);
            this.txtSMTPKey.Name = "txtSMTPKey";
            this.txtSMTPKey.Size = new System.Drawing.Size(339, 31);
            this.txtSMTPKey.TabIndex = 14;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(74, 353);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(117, 25);
            this.label6.TabIndex = 13;
            this.label6.Text = "SMTP key:";
            // 
            // btnTestEmail
            // 
            this.btnTestEmail.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnTestEmail.Location = new System.Drawing.Point(582, 350);
            this.btnTestEmail.Name = "btnTestEmail";
            this.btnTestEmail.Size = new System.Drawing.Size(101, 40);
            this.btnTestEmail.TabIndex = 15;
            this.btnTestEmail.Text = "Test";
            this.btnTestEmail.UseVisualStyleBackColor = true;
            this.btnTestEmail.Click += new System.EventHandler(this.btnTestEmail_Click);
            // 
            // btnHeartbeat
            // 
            this.btnHeartbeat.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnHeartbeat.Location = new System.Drawing.Point(582, 148);
            this.btnHeartbeat.Name = "btnHeartbeat";
            this.btnHeartbeat.Size = new System.Drawing.Size(101, 40);
            this.btnHeartbeat.TabIndex = 16;
            this.btnHeartbeat.Text = "Test";
            this.btnHeartbeat.UseVisualStyleBackColor = true;
            this.btnHeartbeat.Click += new System.EventHandler(this.btnHeartbeat_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(12F, 25F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(718, 563);
            this.Controls.Add(this.btnHeartbeat);
            this.Controls.Add(this.btnTestEmail);
            this.Controls.Add(this.txtSMTPKey);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.txtTimerInterval);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.txtLastHeartbeat);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.txtRWurl);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.txtEmailRecipients);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtRWProcess);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnRestart);
            this.Controls.Add(this.btnStop);
            this.Controls.Add(this.btnStart);
            this.Name = "Form1";
            this.Text = "RW Heartbeat";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnStart;
        private System.Windows.Forms.Button btnStop;
        private System.Windows.Forms.Button btnRestart;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtRWProcess;
        private System.Windows.Forms.TextBox txtEmailRecipients;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.TextBox txtRWurl;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtLastHeartbeat;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txtTimerInterval;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtSMTPKey;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button btnTestEmail;
        private System.Windows.Forms.Button btnHeartbeat;
    }
}

